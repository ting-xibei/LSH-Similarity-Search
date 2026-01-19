#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>  
#include <unordered_set>
#include <random>
#include <numeric>
#include <string>
#include <cstdint>
using namespace std;
#define number_hash 12  // 增加哈希位数提高区分度

// 查询结构体
struct Query {
    vector<int> ids;
    vector<double> vals;
};

// 哈希表节点结构（改为链表法）
struct HashNode {
    string key;
    vector<int> ids;
    HashNode* next;
    HashNode(string k, int id) : key(k), next(nullptr) {
        ids.push_back(id);
    }
};

// 哈希表类（改为链表法实现）
class MyHashTable {
private:
    vector<HashNode*> buckets;
    size_t capacity;

    // FNV-1a哈希函数
    size_t hash_func(const string& key) const {
        const uint32_t FNV_prime = 16777619;
        uint32_t hash = 2166136261;
        for (char c : key) {
            hash ^= c;
            hash *= FNV_prime;
        }
        return hash % capacity;
    }

public:
    MyHashTable(size_t size) : capacity(size) {
        buckets.resize(capacity, nullptr);
    }

    ~MyHashTable() {
        for (auto& head : buckets) {
            while (head) {
                HashNode* temp = head;
                head = head->next;
                delete temp;
            }
        }
    }

    void insert(const string& key, int id) {
        size_t index = hash_func(key);
        HashNode* current = buckets[index];
        
        // 检查是否已存在该key
        while (current) {
            if (current->key == key) {
                current->ids.push_back(id);
                return;
            }
            current = current->next;
        }
        
        // 新建节点并插入链表头部
        HashNode* newNode = new HashNode(key, id);
        newNode->next = buckets[index];
        buckets[index] = newNode;
    }

    vector<int> find(const string& key) {
        size_t index = hash_func(key);
        HashNode* current = buckets[index];
        while (current) {
            if (current->key == key) {
                return current->ids;
            }
            current = current->next;
        }
        return {};
    }
};

// 稀疏向量结构
struct SparseVector {
    vector<int> indices;
    vector<double> values;
    
    // 确保indices是有序的（双指针算法前提）
    void sort_indices() {
        vector<pair<int, double>> paired;
        for (size_t i = 0; i < indices.size(); ++i) {
            paired.emplace_back(indices[i], values[i]);
        }
        sort(paired.begin(), paired.end());
        indices.clear();
        values.clear();
        for (auto& p : paired) {
            indices.push_back(p.first);
            values.push_back(p.second);
        }
    }
};

// 生成随机投影向量
vector<vector<double>> generate_random_vectors(int num_hashes, int dim, unsigned int seed) {
    vector<uint32_t> seed_data{seed};
    seed_seq seq(seed_data.begin(), seed_data.end());
    mt19937 gen(seq);
    normal_distribution<double> dist(0.0, 1.0);

    vector<vector<double>> projections(num_hashes, vector<double>(dim));
    for (auto& vec : projections) {
        for (auto& x : vec) {
            x = dist(gen);
        }
    }
    return projections;
}

// 计算哈希码
vector<bool> compute_hash(const SparseVector& vec, const vector<vector<double>>& projections) {
    vector<bool> hash_code(number_hash, false);
    for (int i = 0; i < number_hash; ++i) {
        double dot = 0.0;
        for (int j = 0; j < vec.indices.size(); ++j) {
            int dim = vec.indices[j];
            dot += vec.values[j] * projections[i][dim];
        }
        hash_code[i] = (dot >= 0);
    }
    return hash_code;
}

// 稀疏向量内积计算（双指针算法）
double sparse_inner_product(const SparseVector& v1, const SparseVector& v2) {
    double result = 0.0;
    int i = 0, j = 0;
    while (i < v1.indices.size() && j < v2.indices.size()) {
        if (v1.indices[i] == v2.indices[j]) {
            result += v1.values[i] * v2.values[j];
            i++;
            j++;
        } else if (v1.indices[i] < v2.indices[j]) {
            i++;
        } else {
            j++;
        }
    }
    return result;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int row, col, nnz, topk;
    cin >> row >> col >> nnz >> topk;
    int num_tables = 5;  // 增加哈希表数量提高召回率

    vector<MyHashTable> hash_tables(num_tables, MyHashTable(1 << number_hash));
    vector<vector<vector<double>>> all_projections(num_tables);
    for (unsigned int i = 0; i < num_tables; ++i) {
        all_projections[i] = generate_random_vectors(number_hash, col, i);
    }

    // 读取稀疏矩阵数据
    vector<int> indptr(row + 1);
    for (int i = 0; i <= row; i++) cin >> indptr[i];
    
    vector<int> indices(nnz);
    for (int i = 0; i < nnz; i++) cin >> indices[i];
    
    vector<double> data(nnz);
    for (int i = 0; i < nnz; i++) cin >> data[i];

    // 构建稀疏向量集合
    vector<SparseVector> sparse_vectors(row);
    for (int i = 0; i < row; ++i) {
        for (int j = indptr[i]; j < indptr[i+1]; ++j) {
            sparse_vectors[i].indices.push_back(indices[j]);
            sparse_vectors[i].values.push_back(data[j]);
        }
        sparse_vectors[i].sort_indices();  // 确保有序
    }

    // 构建哈希表
    for (int i = 0; i < num_tables; ++i) {
        for (int vec_id = 0; vec_id < sparse_vectors.size(); ++vec_id) {
            auto hash_code = compute_hash(sparse_vectors[vec_id], all_projections[i]);
            string hash_key;
            for (bool bit : hash_code) hash_key += (bit ? '1' : '0');
            hash_tables[i].insert(hash_key, vec_id);
        }
    }

    // 处理查询
    int nq;
    cin >> nq;
    vector<Query> queries(nq);
    for (int q = 0; q < nq; q++) {
        int q_nnz;
        cin >> q_nnz;
        queries[q].ids.resize(q_nnz);
        queries[q].vals.resize(q_nnz);
        for (int i = 0; i < q_nnz; i++) cin >> queries[q].ids[i];
        for (int i = 0; i < q_nnz; i++) cin >> queries[q].vals[i];
    }

    for (const auto& query : queries) {
        // 构建查询向量
        SparseVector query_vec;
        query_vec.indices = query.ids;
        query_vec.values = query.vals;
        query_vec.sort_indices();

        // 获取候选集
        unordered_set<int> candidate_ids;
        for (int table_idx = 0; table_idx < num_tables; ++table_idx) {
            auto hash_code = compute_hash(query_vec, all_projections[table_idx]);
            string hash_key;
            for (bool b : hash_code) hash_key += (b ? '1' : '0');
            
            // 查找原始桶
            vector<int> direct_candidates = hash_tables[table_idx].find(hash_key);
            candidate_ids.insert(direct_candidates.begin(), direct_candidates.end());
            
            // 查找邻近桶（翻转1位）
            for (int i = 0; i < hash_key.size() && candidate_ids.size() < 2 * topk; ++i) {
                string neighbor = hash_key;
                neighbor[i] = (neighbor[i] == '1') ? '0' : '1';
                vector<int> neighbor_candidates = hash_tables[table_idx].find(neighbor);
                candidate_ids.insert(neighbor_candidates.begin(), neighbor_candidates.end());
            }
        }

        // 候选集不足时回退到全量搜索
        if (candidate_ids.empty() || candidate_ids.size() < topk * 2) {
            for (int i = 0; i < row; ++i) candidate_ids.insert(i);
        }

        // 计算得分
        vector<pair<double, int>> scores;
        for (int id : candidate_ids) {
            double score = sparse_inner_product(query_vec, sparse_vectors[id]);
            if (score > 0) {  // 只保留正相关结果
                scores.emplace_back(score, id);
            }
        }

        // 部分排序（更高效）
        nth_element(scores.begin(), scores.begin() + min(topk, (int)scores.size()), scores.end(),
            [](const pair<double, int>& a, const pair<double, int>& b) {
                return a.first > b.first || (a.first == b.first && a.second < b.second);
            });

        // 输出结果
        int output_size = min(topk, (int)scores.size());
        for (int i = 0; i < output_size; ++i) {
            if (i != 0) cout << " ";
            cout << scores[i].second;
        }
        cout << endl;
    }
    return 0;
}
