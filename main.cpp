#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <string>
#include <random>
#include <chrono>
#include <filesystem>
#include <cmath>

using namespace std;
namespace fs = std::filesystem;

struct Item {
    int value;
    int weight;
};

int hammingDistance(const vector<bool>& a, const vector<bool>& b) {
    int distance = 0;
    for (size_t i = 0; i < a.size(); i++) {
        if (a[i] != b[i]) {
            distance++;
        }
    }
    return distance;
}

pair<int, int> evaluate(const vector<bool>& solution, const vector<Item>& items) {
    int totalValue = 0;
    int totalWeight = 0;
    for (size_t i = 0; i < solution.size(); i++) {
        if (solution[i]) {
            totalValue += items[i].value;
            totalWeight += items[i].weight;
        }
    }
    return {totalValue, totalWeight};
}

vector<bool> generateRandomSolution(int n, mt19937& gen, uniform_real_distribution<>& dis) {
    vector<bool> solution(n);
    for (int i = 0; i < n; i++) {
        solution[i] = dis(gen) < 0.5;
    }
    return solution;
}

vector<vector<bool>> generateHammingNeighborhood(const vector<bool>& solution, int radius, int maxNeighbors) {
    int n = solution.size();
    vector<vector<bool>> neighborhood;
    for (int i = 0; i < n && neighborhood.size() < maxNeighbors; i++) {
        for (int r = 1; r <= radius; r++) {
            if (neighborhood.size() >= maxNeighbors) break;

            vector<bool> neighbor = solution;
            for (int k = 0; k < r && i + k < n; k++) {
                neighbor[i + k] = !neighbor[i + k];
            }
            neighborhood.push_back(neighbor);
        }
    }

    return neighborhood;
}

vector<bool> hammingSearch(const vector<Item>& items, int capacity, int maxIterations = 1000) {
    int n = items.size();

    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    mt19937 gen(seed);
    uniform_real_distribution<> dis(0.0, 1.0);

    vector<bool> bestSolution = generateRandomSolution(n, gen, dis);
    auto [bestValue, bestWeight] = evaluate(bestSolution, items);
    if (bestWeight > capacity) {
        bestValue = 0;
        fill(bestSolution.begin(), bestSolution.end(), false);
    }

    int hammingRadius = 1;
    int stagnationCounter = 0;

    for (int iteration = 0; iteration < maxIterations; iteration++) {
        auto neighborhood = generateHammingNeighborhood(bestSolution, hammingRadius, 20);

        bool improved = false;

        for (const auto& neighbor : neighborhood) {
            auto [value, weight] = evaluate(neighbor, items);

            if (weight <= capacity) {
                int hDist = hammingDistance(neighbor, bestSolution);

                if (value > bestValue) {
                    bestValue = value;
                    bestSolution = neighbor;
                    improved = true;
                    hammingRadius = 1;
                    stagnationCounter = 0;
                }
                else if (value == bestValue && hDist > n/2) {
                    bestSolution = neighbor;
                    improved = true;
                    stagnationCounter = 0;
                }
            }
        }

        if (!improved) {
            stagnationCounter++;
            if (stagnationCounter > 5) {
                hammingRadius = min(hammingRadius + 1, n);
                stagnationCounter = 0;
            }
        }

        if (iteration % 50 == 0) {
            auto randomSolution = generateRandomSolution(n, gen, dis);
            auto [randValue, randWeight] = evaluate(randomSolution, items);
            if (randWeight <= capacity && randValue > bestValue * 0.7) {
                bestSolution = randomSolution;
                bestValue = randValue;
            }
        }
    }

    return bestSolution;
}

vector<Item> readItemsFromFile(const string& filename) {
    vector<Item> items;
    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "Error: Could not open file: " << filename << endl;
        return items;
    }

    int n, capacity;
    file >> n >> capacity;

    items.resize(n);
    for (int i = 0; i < n; i++) {
        file >> items[i].value >> items[i].weight;
    }

    file.close();
    return items;
}

void writeResultToFile(const string& filename, const vector<bool>& solution, const vector<Item>& items, int capacity, const string& inputFilename) {
    ofstream file(filename);

    if (!file.is_open()) {
        cerr << "Error: Could not create file: " << filename << endl;
        return;
    }

    auto [totalValue, totalWeight] = evaluate(solution, items);

    file << "Knapsack Solution for: " << inputFilename << "\n";
    file << "Capacity: " << capacity << "\n";
    file << "Total value: " << totalValue << "\n";
    file << "Total weight: " << totalWeight << "\n\n";

    file << "Selected items:\n";
    for (size_t i = 0; i < solution.size(); i++) {
        if (solution[i]) {
            file << "Item " << i << ": value=" << items[i].value << ", weight=" << items[i].weight << "\n";
        }
    }

    file << "\nBinary representation:\n";
    for (bool b : solution) {
        file << b;
    }
    file << "\n";

    file.close();
}

int main() {
    string dataFolder = "data";
    string resultsFolder = "results_hamming";

    cout << "Current working directory: " << fs::current_path() << endl;
    cout << "Searching for files in '" << dataFolder << "' folder..." << endl;

    if (!fs::exists(dataFolder)) {
        cerr << "Error: Data folder '" << dataFolder << "' does not exist!" << endl;
        return 1;
    }

    if (!fs::exists(resultsFolder)) {
        fs::create_directory(resultsFolder);
        cout << "Created results folder: " << resultsFolder << endl;
    }

    int fileCount = 0;

    for (const auto& entry : fs::directory_iterator(dataFolder)) {
        if (entry.is_regular_file()) {
            cout << "\nFound file: " << entry.path().filename() << endl;

            string inputPath = entry.path().string();
            string filename = entry.path().filename().string();
            string outputPath = resultsFolder + "/result_" + filename + ".txt";

            ifstream file(inputPath);
            if (!file.is_open()) {
                cerr << "Error: Could not open file" << endl;
                continue;
            }

            int n, capacity;
            file >> n >> capacity;

            vector<Item> items(n);
            for (int i = 0; i < n; i++) {
                file >> items[i].value >> items[i].weight;
            }
            file.close();

            cout << "Read " << n << " items, capacity: " << capacity << endl;

            vector<bool> bestSolution = hammingSearch(items, capacity);
            auto [bestValue, bestWeight] = evaluate(bestSolution, items);

            writeResultToFile(outputPath, bestSolution, items, capacity, filename);

            cout << "Best value: " << bestValue << ", weight: " << bestWeight << endl;
            cout << "Result saved to: result_" << filename << ".txt" << endl;

            fileCount++;
        }
    }

    cout << "Processing complete!" << endl;
    cout << "Processed " << fileCount << " files." << endl;
    cout << "Results saved in '" << resultsFolder << "' folder." << endl;

    return 0;
}
