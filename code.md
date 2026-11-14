# FINAL CODE

## Traffic Load Balancing with Road Capacity
### Open-Ended Lab Task

Every road can carry at most `CAPACITY = 100` vehicles per time step.

---

## Solution Approaches

### 1. Shortest Path
- All vehicles are assigned to the shortest path.
- Ignores existing traffic and road capacity.

### 2. Greedy Load Balancing (Greedy LB)
- Vehicles are divided into small groups.
- Each group takes the least-loaded available path.
- Ignores travel distance.
- Fully occupied roads are skipped.

### 3. Proposed Approach: CALB (Congestion-Aware Load Balancing)

CALB uses an optimal minimum-cost flow split for each request.

#### Cost Function

The cost of assigning the *k-th* vehicle to a road is:

\[
\text{Cost}(k) = w \times (1 + \text{ALPHA} \times (k-1))
\]

Where:
- `w` = base weight of the road.
- `ALPHA` = congestion penalty factor.
- `k` = vehicle's position on the road.

As more vehicles use a road, its cost increases. A fully occupied road (`100` vehicles) cannot be used.

#### Method: Successive Shortest Paths

1. Vehicles are sent one at a time through the cheapest available path in the residual network.
2. The residual network can also reroute previously assigned vehicles of the same request to alternative routes.
3. This allows CALB to find an **optimal vehicle distribution for each request**, given the existing traffic on the roads.

#### Iterative Re-optimization

After processing all requests, CALB re-optimizes them one by one until no request can further reduce the total cost.

**Note:** A guaranteed global optimum across all requests jointly is NP-hard in general.