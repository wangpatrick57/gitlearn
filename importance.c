#include <stdio.h>
#include "importance.h"
#include <assert.h>

int *enumerateImportanceDegreeOrder(GRAPH *G) {
    double importances[G->n];
    getImportances(importances, G);
    node_wdegree arrForSorting[G->n];
    int i;

    for (i = 0; i < G->n; ++i) {
        arrForSorting[i].node = i;
        arrForSorting[i].degree_val = importances[i];
    }

    qsort((void*)arrForSorting, G->n, sizeof(node_wdegree), nwd_asccompFunc);

    int* degreeOrder = malloc(sizeof(int) * G->n);

    for (i = 0; i < G->n; ++i) {
        degreeOrder[arrForSorting[i].node] = i;
    }

    return degreeOrder;
}

void getImportances(double *importances, GRAPH *G) {
    int i, j, k, n = G->n;
    double edgeWeights[n][n];
    double nodeWeights[n];

    for (i = 0; i < n; ++i) {
        nodeWeights[i] = 0;

        for (j = 0; j < n; ++j) {
            if (GraphAreConnected(G, i, j)) {
                edgeWeights[i][j] = 1.0;
            } else {
                edgeWeights[i][j] = 0;
            }
        }
    }

    double edgeWeightSum = 0;

    for (i = 0; i < n; ++i) {
        for (j = 0; j < n; ++j) {
            edgeWeightSum += edgeWeights[i][j];
        }
    }

    fprintf(stderr, "edgeWeightSum: %f\n", edgeWeightSum);

    // we need to make a copy of G->neighbor because we're going to modify it
    // however, we don't need to make a copy of G->degree because that won't change
    ADJ_LIST *adjList = generateAdjacencyList(G);
    int sortedNodes[n];
    fillSortedNodes(sortedNodes, G);
    
    for (i = 0; i < n; ++i) {
        assert(G->degree[i] == adjList->sizes[i]);

        for (j = 0; j < G->degree[i]; ++j) {
            assert(G->neighbor[i][j] == adjList->lists[i][j]);
        }
    }

    for (i = 0; i < n; ++i) {
        if (G->degree[sortedNodes[i]] > IMPORTANCE_DEG) {
            break;
        }

        int u = sortedNodes[i];

        // update neighbors' weights
        if (G->degree[u] == 1) {
            for (j = 0; j < adjList->sizes[u]; ++j) {
                int v = adjList->lists[u][j];
                nodeWeights[v] += nodeWeights[u] + edgeWeights[u][v];
            }
        } else {
            double uWeight = nodeWeights[u];

            for (j = 0; j < adjList->sizes[u]; ++j) {
                int v = adjList->lists[u][j];
                uWeight += edgeWeights[u][v];
            }

            for (j = 0; j < adjList->sizes[u]; ++j) {
                int v1 = adjList->lists[u][j];

                for (k = j + 1; k < adjList->sizes[u]; ++k) {
                    int v2 = adjList->lists[u][k];
                    double numNeighbors = adjList->sizes[u];
                    double edgeWeightInc = uWeight / ((numNeighbors * (numNeighbors - 1)) / 2.0);
                    edgeWeights[v1][v2] += edgeWeightInc;
                    edgeWeights[v2][v1] += edgeWeightInc;
                }
            }
        }

        for (j = 0; j < adjList->sizes[u]; ++j) {
            int v = adjList->lists[u][j];
            removeFromAdjacencyList(adjList, v, u);
        }

        // nodeWeights[u] = 0;
        /* The paper says: "When one node is removed, its adjacent
        edges are also removed and the weight of the removed node and
        edges are allocated to their neighboring nodes and edges. In this
        way, the topological information is propagated from a node to
        its neighbors."
        It is not clear whether when a node is removed it loses its weight.
        The wording seems to indicate so, but then all the nodes with degree <=10 (DEG)
        would end up with weight 0, and many mappings would be random (this is no longer
        true when also adding sequnece similarity).
        I assume here that they don't keep their weight (to change this, comment the previous line)
        Note: in a test, the scores were similar in both cases */

        /* same story with edge weights... */
        /* for (j = 0; j < n; j++) {
            edgeWeights[u][j] = 0;
            edgeWeights[j][u] = 0;
            } */
    }

    // compute importances
    int u;

    for (u = 0; u < n; ++u) {
        double edgeWeightSum = 0;

        for (i = 0; i < G->degree[u]; ++i) {
            int v = G->neighbor[u][i];
            edgeWeightSum += edgeWeights[u][v];
        }

        importances[u] = nodeWeights[u] + edgeWeightSum;
    }

    // normalizeImportances(importances);

    freeAdjacencyList(adjList, G);
}

ADJ_LIST *generateAdjacencyList(GRAPH *G) {
    ADJ_LIST *adjList = malloc(sizeof(ADJ_LIST*));
    adjList->lists = malloc(G->n * sizeof(int*));
    adjList->sizes = malloc(G->n * sizeof(int));
    int basei, neighi;

    for (basei = 0; basei < G->n; ++basei) {
        adjList->lists[basei] = malloc(G->degree[basei] * sizeof(int));
        adjList->sizes[basei] = G->degree[basei];

        for (neighi = 0; neighi < G->degree[basei]; ++neighi) {
            adjList->lists[basei][neighi] = G->neighbor[basei][neighi];
        }
    }

    return adjList;
}

void freeAdjacencyList(ADJ_LIST *adjList, GRAPH *G) {
    int i;

    for (i = 0; i < G->n; ++i) {
        free(adjList->lists[i]);
    }

    free(adjList->sizes);
    free(adjList);
}

void fillSortedNodes(int *sortedNodes, GRAPH *G) {
    node_wdegree arrForSorting[G->n];
    int i, j;

    for (i = 0; i < G->n; ++i) {
        arrForSorting[i].node = i;
        arrForSorting[i].degree_val = G->degree[i];
    }

    qsort((void*)arrForSorting, G->n, sizeof(node_wdegree), nwd_asccompFunc);

    for (i = 0; i < G->n; ++i) {
        sortedNodes[i] = arrForSorting[i].node;
    }
}

void removeFromAdjacencyList(ADJ_LIST *adjList, int base, int neigh) {
    assert(adjList->sizes[base] > 0);
    int i;
    int neighPos;

    for (i = 0; i < adjList->sizes[base]; ++i) {
        if (adjList->lists[base][i] == neigh) {
            neighPos = i;
            break;
        }
    }

    adjList->lists[base][neighPos] = adjList->lists[base][adjList->sizes[base] - 1]; // one way swap
    adjList->sizes[base]--; // decrement size
}
