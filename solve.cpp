#include <bits/stdc++.h>
#include <iostream>
#include <vector>
#include <queue>
#include <deque>
#include <map>
#include <set>
#include <random>
#include <chrono>
#include <iomanip>
#include <functional>
#include <algorithm>
using namespace std;

const int CAPACITY = 100;              // max vehicles on one road per time step
double ALPHA = 0.1;                    // how strongly traffic raises the cost
const double INF = 1e18;
const char* NAMES[3] = {"Shortest Path", "Greedy LB", "Proposed"};

struct Edge { int u, v, w; };
struct Request { int n, s, d; };       // n vehicles from s to d

struct Graph {
    int V;
    vector<Edge> edges;
    vector<vector<pair<int,int>>> adj; // (neighbour, edge index)
    int other(int e, int x) const { return edges[e].u == x ? edges[e].v : edges[e].u; }
};

// ===================== 1. Generate network and requests =====================

Graph generateGraph(int V, int extraEdges, mt19937& rng) {
    Graph g; g.V = V; g.adj.resize(V);
    set<pair<int,int>> used;                          // no two roads between same pair
    auto addEdge = [&](int u, int v) {
        if (u == v || !used.insert({min(u, v), max(u, v)}).second) return;
        int w = rng() % 20 + 1;                       // distance 1..20
        g.adj[u].push_back({v, (int)g.edges.size()});
        g.adj[v].push_back({u, (int)g.edges.size()});
        g.edges.push_back({u, v, w});
    };
    for (int i = 1; i < V; i++) addEdge(i, rng() % i);       // random tree -> connected
    for (int k = 0; k < extraEdges; k++) addEdge(rng() % V, rng() % V);
    return g;
}

vector<vector<Request>> generateRequests(int V, int T, int R, int maxN, mt19937& rng) {
    vector<vector<Request>> steps(T);
    for (int t = 0; t < T; t++)
        for (int i = 0; i < R; i++) {
            int s = rng() % V, d = rng() % V;
            while (d == s) d = rng() % V;
            steps[t].push_back({(int)(rng() % maxN) + 1, s, d});
        }
    return steps;
}

// ===================== 2. Dijkstra (for the two baselines) =====================
// cost(e) < 0 means the road is blocked. Returns false if d cannot be reached.
bool dijkstra(const Graph& g, int s, int d, function<double(int)> cost, vector<int>& path) {
    vector<double> dist(g.V, INF);
    vector<int> parentEdge(g.V, -1);
    priority_queue<pair<double,int>, vector<pair<double,int>>, greater<>> pq;
    dist[s] = 0; pq.push({0, s});
    while (!pq.empty()) {
        auto [du, u] = pq.top(); pq.pop();
        if (du > dist[u]) continue;
        if (u == d) break;
        for (auto [v, e] : g.adj[u]) {
            double c = cost(e);
            if (c < 0) continue;
            if (du + c < dist[v]) { dist[v] = du + c; parentEdge[v] = e; pq.push({dist[v], v}); }
        }
    }
    if (dist[d] >= INF) return false;
    path.clear();
    for (int v = d; v != s; v = g.other(parentEdge[v], v)) path.push_back(parentEdge[v]);
    reverse(path.begin(), path.end());
    return true;
}

// ===================== 3. Proposed: optimal split of one request =====================
// flow[e] = this request's vehicles on road e (+ means u->v, - means v->u)
// load[e] = all vehicles on road e (other requests + this one)
//
// Cost of moving one vehicle across road e in direction dir:
//   - same direction as this request's flow (or road unused): one more vehicle,
//     costs w*(1 + ALPHA*load), blocked if the road is already full
//   - against this request's flow: takes one of its vehicles OFF the road,
//     saves w*(1 + ALPHA*(load-1))  -> negative cost (lets the method reroute)
double arcCost(const Graph& g, int e, int dir, const vector<int>& flow, const vector<int>& load) {
    double w = g.edges[e].w;
    if (flow[e] * dir >= 0) {
        if (load[e] >= CAPACITY) return INF;              // road full
        return w * (1 + ALPHA * load[e]);
    }
    return -w * (1 + ALPHA * (load[e] - 1));
}

// Returns how many vehicles could NOT be routed (all paths full).
int optimalSplit(const Graph& g, const Request& r, vector<int>& load, map<vector<int>, int>& paths) {
    int E = g.edges.size();
    vector<int> flow(E, 0);
    int sent = 0;
    for (; sent < r.n; sent++) {
        // Cheapest path in the residual network (Bellman-Ford / SPFA, handles negative arcs)
        vector<double> dist(g.V, INF);
        vector<int> parentEdge(g.V, -1);
        vector<char> inQueue(g.V, 0);
        deque<int> q;
        dist[r.s] = 0; q.push_back(r.s);
        while (!q.empty()) {
            int x = q.front(); q.pop_front(); inQueue[x] = 0;
            for (auto [y, e] : g.adj[x]) {
                int dir = (g.edges[e].u == x) ? 1 : -1;
                double c = arcCost(g, e, dir, flow, load);
                if (c >= INF) continue;
                if (dist[x] + c < dist[y] - 1e-9) {
                    dist[y] = dist[x] + c; parentEdge[y] = e;
                    if (!inQueue[y]) { inQueue[y] = 1; q.push_back(y); }
                }
            }
        }
        if (dist[r.d] >= INF) break;                      // no free path left
        // Send one vehicle along that path
        for (int y = r.d; y != r.s; ) {
            int e = parentEdge[y], x = g.other(e, y);
            int dir = (g.edges[e].u == x) ? 1 : -1;
            load[e] += (flow[e] * dir >= 0) ? 1 : -1;
            flow[e] += dir;
            y = x;
        }
    }
    // Split the flow into separate paths with their vehicle counts
    paths.clear();
    while (true) {
        vector<int> path;
        vector<char> seen(g.V, 0);
        int x = r.s, bottleneck = INT32_MAX;
        while (x != r.d) {
            seen[x] = 1;
            int next = -1;
            for (auto [y, e] : g.adj[x]) {
                int dir = (g.edges[e].u == x) ? 1 : -1;
                if (flow[e] * dir > 0 && !seen[y]) { next = e; break; }
            }
            if (next == -1) break;
            path.push_back(next);
            bottleneck = min(bottleneck, abs(flow[next]));
            x = g.other(next, x);
        }
        if (x != r.d || path.empty()) break;
        int y = r.s;
        for (int e : path) {
            int dir = (g.edges[e].u == y) ? 1 : -1;
            flow[e] -= dir * bottleneck;
            y = g.other(e, y);
        }
        paths[path] += bottleneck;
    }
    return r.n - sent;
}

// ===================== 4. Route one time step =====================

struct StepResult {
    vector<int> load;                       // vehicles on each road
    vector<map<vector<int>, int>> paths;    // per request: path -> vehicles
    vector<int> unrouted;                   // per request: vehicles with no free path
};

StepResult routeStep(const Graph& g, const vector<Request>& reqs, int method) {
    StepResult res;
    res.load.assign(g.edges.size(), 0);
    res.paths.resize(reqs.size());
    res.unrouted.assign(reqs.size(), 0);

    if (method == 2) {                      // ---- Proposed ----
        for (size_t i = 0; i < reqs.size(); i++)
            res.unrouted[i] = optimalSplit(g, reqs[i], res.load, res.paths[i]);
        // Re-optimise each request against everyone else until nothing changes
        for (int pass = 0; pass < 10; pass++) {
            bool changed = false;
            for (size_t i = 0; i < reqs.size(); i++) {
                for (auto& [path, veh] : res.paths[i])
                    for (int e : path) res.load[e] -= veh;      // take request i off the roads
                map<vector<int>, int> old = res.paths[i];
                res.unrouted[i] = optimalSplit(g, reqs[i], res.load, res.paths[i]);
                if (res.paths[i] != old) changed = true;
            }
            if (!changed) break;
        }
        return res;
    }

    for (size_t i = 0; i < reqs.size(); i++) {   // ---- Baselines ----
        Request r = reqs[i];
        int group = (method == 0) ? r.n : max(1, (r.n + 4) / 5);
        int left = r.n;
        while (left > 0) {
            int c = min(group, left);
            function<double(int)> cost;
            if (method == 0)   // shortest path: distance only, capacity ignored
                cost = [&](int e) { return (double)g.edges[e].w; };
            else               // greedy: least loaded road, skip roads that would overflow
                cost = [&](int e) {
                    if (res.load[e] + c > CAPACITY) return -1.0;
                    return res.load[e] + 1 + g.edges[e].w * 1e-6;
                };
            vector<int> path;
            if (!dijkstra(g, r.s, r.d, cost, path)) {
                if (group > 1) { group = 1; continue; }   // try single vehicles
                break;                                     // no free path at all
            }
            for (int e : path) res.load[e] += c;
            res.paths[i][path] += c;
            left -= c;
        }
        res.unrouted[i] = left;
    }
    return res;
}

// ===================== 5. Metrics =====================

struct Metrics {
    double maxLoad = 0, avgLoad = 0, variance = 0, totalDist = 0, timeMs = 0;
    int overloaded = 0, unrouted = 0;
};

Metrics evaluate(const Graph& g, const StepResult& res) {
    Metrics m;
    int E = g.edges.size();
    for (int l : res.load) {
        m.maxLoad = max(m.maxLoad, (double)l);
        m.avgLoad += l;
        if (l > CAPACITY) m.overloaded++;
    }
    m.avgLoad /= E;
    for (int l : res.load) m.variance += (l - m.avgLoad) * (l - m.avgLoad);
    m.variance /= E;
    for (auto& req : res.paths)
        for (auto& [path, veh] : req) {
            int len = 0;
            for (int e : path) len += g.edges[e].w;
            m.totalDist += (double)veh * len;
        }
    for (int u : res.unrouted) m.unrouted += u;
    return m;
}

void addTo(Metrics& total, const Metrics& m, int steps) {
    total.maxLoad = max(total.maxLoad, m.maxLoad);        // worst step
    total.avgLoad += m.avgLoad / steps;                   // average over steps
    total.variance += m.variance / steps;
    total.totalDist += m.totalDist;
    total.timeMs += m.timeMs;
    total.overloaded += m.overloaded;
    total.unrouted += m.unrouted;
}

// ===================== 6. Printing =====================

void printStep(const Graph& g, const vector<Request>& reqs, const StepResult& res) {
    for (size_t i = 0; i < reqs.size(); i++) {
        cout << "  Request " << i + 1 << ": " << reqs[i].n << " vehicles, "
             << reqs[i].s << " -> " << reqs[i].d << "\n";
        for (auto& [path, veh] : res.paths[i]) {
            int v = reqs[i].s, len = 0;
            cout << "     " << setw(3) << veh << " vehicles : " << v;
            for (int e : path) { v = g.other(e, v); len += g.edges[e].w; cout << " -> " << v; }
            cout << "   (distance " << len << ")\n";
        }
        if (res.unrouted[i] > 0)
            cout << "     " << setw(3) << res.unrouted[i] << " vehicles : NOT ROUTED (all paths full)\n";
    }
    cout << "  Road loads (capacity " << CAPACITY << "):\n";
    for (size_t e = 0; e < g.edges.size(); e++)
        if (res.load[e] > 0)
            cout << "     road " << g.edges[e].u << "-" << g.edges[e].v << " (w=" << g.edges[e].w
                 << ") : " << res.load[e] << (res.load[e] > CAPACITY ? "   <-- OVER CAPACITY" : "") << "\n";
}

void printTableHeader(const string& first = "Algorithm") {
    cout << left << setw(16) << first << right << setw(9) << "MaxLoad" << setw(9) << "AvgLoad"
         << setw(11) << "Variance" << setw(11) << "TotalDist" << setw(11) << "Overload"
         << setw(10) << "Unrouted" << setw(10) << "Time(ms)" << "\n";
    cout << string(87, '-') << "\n";
}

void printRow(const string& name, const Metrics& m) {
    cout << left << setw(16) << name << right << fixed << setprecision(2)
         << setw(9) << m.maxLoad << setw(9) << m.avgLoad << setw(11) << m.variance
         << setw(11) << setprecision(0) << m.totalDist << setw(11) << m.overloaded
         << setw(10) << m.unrouted << setw(10) << setprecision(2) << m.timeMs << "\n";
}

// Run all 3 algorithms on the same instance
void runAll(const Graph& g, const vector<vector<Request>>& steps, bool details) {
    Metrics total[3];
    for (int a = 0; a < 3; a++)
        for (size_t t = 0; t < steps.size(); t++) {
            auto start = chrono::steady_clock::now();
            StepResult res = routeStep(g, steps[t], a);
            double ms = chrono::duration<double, milli>(chrono::steady_clock::now() - start).count();
            if (details) {
                cout << "\n=== " << NAMES[a] << " | time step " << t + 1 << " ===\n";
                printStep(g, steps[t], res);
            }
            Metrics m = evaluate(g, res);
            m.timeMs = ms;
            addTo(total[a], m, steps.size());
        }
    cout << "\n===== COMPARISON (" << g.V << " nodes, " << g.edges.size() << " roads, "
         << steps.size() << " time steps, capacity " << CAPACITY << ") =====\n";
    printTableHeader();
    for (int a = 0; a < 3; a++) printRow(NAMES[a], total[a]);
    cout << "Overload = roads above capacity (summed over steps), Unrouted = vehicles with no free path\n";
}

// ===================== main menu =====================

int main() {
    mt19937 rng(42);
    while (true) {
        cout << "\n========== TRAFFIC LOAD BALANCING LAB (capacity " << CAPACITY << ") ==========\n"
             << " 1. Small example (shows paths, vehicles, road loads)\n"
             << " 2. Custom network size\n"
             << " 3. Trade-off test (change alpha)\n"
             << " 4. Scalability test (running time)\n"
             << " 0. Exit\n"
             << "Choice: ";
        int ch;
        if (!(cin >> ch) || ch == 0) break;

        if (ch == 1) {
            Graph g = generateGraph(10, 8, rng);
            auto steps = generateRequests(10, 2, 4, 80, rng);
            runAll(g, steps, true);
        }
        else if (ch == 2) {
            int V, extra, T, R, maxN;
            cout << "Number of nodes: ";            cin >> V;
            cout << "Extra roads (besides tree): "; cin >> extra;
            cout << "Time steps: ";                 cin >> T;
            cout << "Requests per step: ";          cin >> R;
            cout << "Max vehicles per request: ";   cin >> maxN;
            if (!cin || V < 2 || extra < 0 || T < 1 || R < 1 || maxN < 1) {
                cout << "Invalid input.\n"; cin.clear(); cin.ignore(10000, '\n'); continue;
            }
            char show;
            cout << "Show detailed paths? (y/n): "; cin >> show;
            Graph g = generateGraph(V, extra, rng);
            auto steps = generateRequests(V, T, R, maxN, rng);
            runAll(g, steps, show == 'y' || show == 'Y');
        }
        else if (ch == 3) {
            Graph g = generateGraph(200, 300, rng);
            auto steps = generateRequests(200, 3, 30, 60, rng);
            cout << "\nProposed method with different alpha (200 nodes, 3 time steps)\n"
                 << "alpha = 0: shortest paths, only capacity forces detours; bigger alpha = more balancing\n";
            printTableHeader("alpha");
            double saved = ALPHA;
            for (double a : {0.0, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0}) {
                ALPHA = a;
                Metrics total;
                for (auto& st : steps) {
                    auto start = chrono::steady_clock::now();
                    Metrics m = evaluate(g, routeStep(g, st, 2));
                    m.timeMs = chrono::duration<double, milli>(chrono::steady_clock::now() - start).count();
                    addTo(total, m, steps.size());
                }
                printRow("alpha=" + to_string(a).substr(0, 4), total);
            }
            ALPHA = saved;
        }
        else if (ch == 4) {
            cout << "\nRunning time for growing networks (3 steps, 50 requests each)\n";
            for (int V : {100, 500, 1000, 2000, 5000}) {
                Graph g = generateGraph(V, V / 2, rng);
                auto steps = generateRequests(V, 3, 50, 30, rng);
                runAll(g, steps, false);
            }
        }
    }
    cout << "Bye!\n";
    return 0;
}