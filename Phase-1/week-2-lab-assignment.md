```cpp
struct CSRGraph {
    int num_vertices;
    std::vector<int> offsets;
    std::vector<int> edges;
};

CSRGraph LoadGraph(const char *filename);
```

Implement the `LoadGraph` function to read the graph from the provided edge list file ([soc-Slashdot0902.txt](https://drive.google.com/drive/folders/1Cr4QkLBpWa3Gp0u-9YWNE9voH9Dz4INg)) and construct the CSR representation.

Please submit your code (copy/paste your implementation) and execution result on canvas (an image to show the execution result, e.g., given a CSRGraph, print the neighbors of vertex 6).
