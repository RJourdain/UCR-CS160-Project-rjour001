```cpp
struct CSRGraph {
    int num_vertices;
    std::vector<int> offsets;
    std::vector<int> edges;
};

CSRGraph LoadGraph(const char *filename);
```

Implement the `LoadGraph` function to read the graph from the provided `edges.txt` file and construct the CSR representation.