FINAL CODE 

Traffic Load Balancing with Road Capacity - Open Ended Lab Task

Every road can carry at most CAPACITY = 100 vehicles per time step.

Solution Approaches :: 

1. Shortest Path : all vehicles on the shortest path (ignores traffic and capacity)

2. Greedy LB     : small groups, each takes the least-loaded path (ignores distance), full roads are skipped

3. Proposed      : CALB (Congestion-Aware Load Balancing)
   --> optimal min-cost flow split for each request.
   --> method : 
   The k-th vehicle on a road costs  w * (1 + ALPHA * (k-1)),
   so a busy road gets more expensive with every extra vehicle, and a full road
   (100 vehicles) cannot be used at all.
   Vehicles are sent one at a time on the cheapest path in the residual network
   (successive shortest paths). The residual network can also move earlier
   vehicles of the same request to another route, so the split found for a
   request is OPTIMAL for that request, given the traffic already on the roads.
   Then the requests are re-optimised one by one until no request can lower
   the cost any more.
   (A guaranteed optimum over ALL requests jointly is NP-hard in general.)