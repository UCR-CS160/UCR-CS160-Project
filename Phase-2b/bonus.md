# Phase 2 Bonus: Direction-Optimizing Push/Pull (extra 3 pt)

Push is cheap when only a few vertices have anything to push. Pull's advantage is that each `v` is written by only one thread, so no locks are needed. BFS naturally starts with a tiny active set, grows to cover much of the graph in the middle rounds, then shrinks again, so the cheaper direction changes round by round. The idea: at the end of each round, count how many vertices changed, flip a mode flag, and have `Process()` dispatch to a push body or a pull body. You may reuse most of code from Phase 2a/2b.

## Task

Implement the above idea for BFS, SSSP, and CC, and compare the performance against your push-only (Phase 2b) and pull-only (Phase 2a) implementations. You have two weeks for this, and you may submit it together with Phase 2c.
