#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <random>
#include <numeric>
#include <utility>
using namespace std;

#define NUMBER_HASH 12

// 快速哈希表（开放寻址法）
class FastHashTable {
private:
    vector<pair<uint32_t, vector<int>>> table;  // 存储哈希值和对应的向量ID列表
    size_t capacity;
    
    // 高效的哈希函数
    uint32_t hash_func(uint32_t key) {
        key = ((key >> 16) ^ key) * 0x45d9f3b;
        return key % capacity;
    }

public:
    FastHashTable(size_t size) : capacity(size * 2) {  // 负载因子设为0.5
        table.resize(capacity);
    }

    // 插入键值对
    void insert(uint32_t hash_val, int vec_id) {
        size_t pos = hash_val % capacity;
        while (table[pos].second.size() != 0 && table[pos].first != hash_val) {
            pos = (pos + 1) % capacity;  // 线性探测
        }
        if (table[pos].second.empty()) {
            table[pos].first = hash_val;  // 新键
        }
        table[pos].second.push_back(vec_id);
    }

    // 查找键对应的值
    vector<int> find(uint32_t hash_val) {
        size_t pos = hash_val % capacity;
        while (table[pos].second.size() != 0) {
            if (table[pos].first == hash_val) return table[pos].second;
            pos = (pos + 1) % capacity;
        }
        return {};
    }
};

// 将bool哈希码向量转为uint32整数键
uint32_t bool_vector_to_int(const vector<bool>& hash_code) {
    uint32_t res = 0;
    for (int i = 0; i < min(32, (int)hash_code.size()); ++i) {
        if (hash_code[i]) res |= (1 << i);  // 位操作加速
    }
    return res;
}

// 稀疏向量结构
struct SparseVector {
    vector<int> indices;    // 非零元素的索引
    vector<double> values;  // 对应的非零值
    int dimension;          // 向量原始维度

    SparseVector(int dim = 0) : dimension(dim) {}
    
    // 对索引进行排序（双指针算法要求有序）
    void sort_indices() {
        vector<pair<int, double>> paired;
        for (size_t i = 0; i < indices.size(); ++i) {
            paired.emplace_back(indices[i], values[i]);
        }
        sort(paired.begin(), paired.end());
        for (size_t i = 0; i < paired.size(); ++i) {
            indices[i] = paired[i].first;
            values[i] = paired[i].second;
        }
    }
};

// 生成随机投影矩阵（稀疏优化版）
vector<vector<double>> generate_random_vectors(int num_hashes, int dim) {
    vector<uint32_t> seed_data{42};  
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

// 计算向量的SRP哈希码（密集向量版）
vector<bool> compute_hash(const vector<double>& vec, 
                         const vector<vector<double>>& projections) {
    vector<bool> hash_code(NUMBER_HASH, false);
    for (int i = 0; i < min(NUMBER_HASH, (int)projections.size()); ++i) {
        double dot = inner_product(vec.begin(), vec.end(), 
                                 projections[i].begin(), 0.0);
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

    /* 输入数据 */
    int row, col, nnz, topk;
    cin >> row >> col >> nnz >> topk;
    int num_tables = 5;  // 哈希表数量

    // 初始化哈希表
    vector<FastHashTable> hash_tables(num_tables, FastHashTable(1 << 20));
    vector<vector<vector<double>>> all_projections(num_tables); 
    for (unsigned int i = 0; i < num_tables; ++i) {
        vector<uint32_t> seed_data{i};  // 不同哈希表不同随机种子
        seed_seq seq(seed_data.begin(), seed_data.end());
        mt19937 gen(seq);  
        all_projections[i] = generate_random_vectors(NUMBER_HASH, col);
    }

    /* 读取稀疏矩阵数据（CSR格式） */
    vector<int> indptr(row + 1);
    for (int i = 0; i <= row; i++) cin >> indptr[i];
    
    vector<int> indices(nnz);
    for (int i = 0; i < nnz; i++) cin >> indices[i];
    
    vector<double> data(nnz);
    for (int i = 0; i < nnz; i++) cin >> data[i];

    /* 构建稀疏向量集合 */
    vector<vector<double>> dense_vectors(row, vector<double>(col, 0.0));
    vector<SparseVector> sparse_vectors(row);
    for (int i = 0; i < row; ++i) {
        sparse_vectors[i].dimension = col;
        for (int j = indptr[i]; j < indptr[i+1]; ++j) {
            sparse_vectors[i].indices.push_back(indices[j]);
            sparse_vectors[i].values.push_back(data[j]);
            dense_vectors[i][indices[j]] = data[j];  // 保持密集表示兼容
        }
        sparse_vectors[i].sort_indices();  // 确保有序
    }

    /* 构建哈希表 */
    for (int i = 0; i < num_tables; ++i) {
        for (int j = 0; j < row; ++j) {
            auto hash_code = compute_hash(dense_vectors[j], all_projections[i]);
            uint32_t hash_val = bool_vector_to_int(hash_code);
            hash_tables[i].insert(hash_val, j);
        }
    }

    /* 处理查询 */
    int nq;
    cin >> nq;
    while (nq--) {
        int q_nnz;
        cin >> q_nnz;
        
        // 构建查询向量（两种表示）
        vector<double> query_dense(col, 0.0);
        SparseVector query_sparse(col);
        query_sparse.indices.resize(q_nnz);
        query_sparse.values.resize(q_nnz);
        
        for (int i = 0; i < q_nnz; i++) cin >> query_sparse.indices[i];
        for (int i = 0; i < q_nnz; i++) cin >> query_sparse.values[i];
        
        for (int i = 0; i < q_nnz; ++i) {
            query_dense[query_sparse.indices[i]] = query_sparse.values[i];
        }
        query_sparse.sort_indices();

        /* 获取候选集 */
        unordered_set<int> candidate_ids;
        for (int table_idx = 0; table_idx < num_tables; ++table_idx) {
            auto hash_code = compute_hash(query_dense, all_projections[table_idx]);
            uint32_t hash_val = bool_vector_to_int(hash_code);
            for (int id : hash_tables[table_idx].find(hash_val)) {
                candidate_ids.insert(id);
            }
        }

        // 候选集不足时回退到全量
        if (candidate_ids.empty() || candidate_ids.size() < topk * 2) {
            for (int i = 0; i < row; ++i) candidate_ids.insert(i);
        }

        /* 计算内积得分 */
        vector<pair<double, int>> scores;
        for (int id : candidate_ids) {
            double score = sparse_inner_product(query_sparse, sparse_vectors[id]);
            scores.emplace_back(score, id);
        }

        /* 部分排序 */
        nth_element(scores.begin(), scores.begin() + min(topk, (int)scores.size()), scores.end(),
            [](const pair<double, int>& a, const pair<double, int>& b) {
                return a.first > b.first || (a.first == b.first && a.second < b.second);
            });

        /* 输出结果 */
        int output_size = min(topk, (int)scores.size());
        for (int i = 0; i < output_size; ++i) {
            if (i != 0) cout << " ";
            cout << scores[i].second;
        }
        cout << endl;
    }
}
