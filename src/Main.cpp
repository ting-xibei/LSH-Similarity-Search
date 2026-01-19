
#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>  
#include<unordered_set>
#include <random>
#include <numeric>
#include<string>
#include<cstdint>
using namespace std;
#define number_hash  8

struct HashNode{
    string key;     //键——哈希码
    vector<int>ids;//向量id【lsh中需要一个键对应多个向量id ,才能找出所有候选向量】
    bool occupied=0;
    bool deleted= 0;
    HashNode() : key(""), ids({}), occupied(false), deleted(false) {}
    
    HashNode(const string &k, int id) :
    key(k), ids({id}), occupied(true), deleted(false) {}  // 注意ids初始化
    
};

class MyHashTable{
    private:
    vector<HashNode>table;//存储哈希表
    size_t capacity;
    size_t size=0;  //当前元素数
    

    //哈希函数DJB2
    size_t hash_func(const string &key)const {
        size_t hash=5381;//魔术数
        for(char c:key){
            hash=((hash<<5)+hash)+c;
        }
        return hash%capacity;
    }


    //平方探查函数
    size_t probe(size_t index,size_t attempt)const {
        return (index+attempt*attempt)%capacity;
    }

    //辅助函数rehash(扩容)
    void rehash(){
        size_t new_capacity=capacity*2;
        vector<HashNode>new_table(new_capacity);
        size_t new_size=0;
        //遍历旧表中的元素
        for(auto&node:table){
            //已经占用和未删除的点
            if(node.occupied&&!node.deleted){
                size_t hash_val=hash_func(node.key);//缓存这个数避免重复计算
                size_t index=hash_val%new_capacity;//计算新位置
                size_t attempt=0;

                while(attempt<log2(new_capacity))//平方探测
                {
                    size_t current = (index + attempt * attempt) % new_capacity;
                    //新表可放时才放
                    if (!new_table[current].occupied || new_table[current].deleted) {
                        
                        new_table[current] = node;//插入
                        new_size++;
                        break;
                    }
                    attempt++;
                }
            }
        }
    }

    public:

   MyHashTable(size_t volume) : capacity(volume), size(0) {  // 初始化
    table.resize(capacity, HashNode("", -1));  // 默认构造值
}

    //插入操作
    void insert(const string &key,int id){
        if(size>=capacity*0.8)//用装填因子来动态扩容
        {
            rehash();
        }
        size_t index=hash_func(key);
        size_t attempt=0;
        while(attempt<log2(capacity)){

            size_t current = probe(index, attempt);
            //可以填入
            if (!table[current].occupied || table[current].deleted) {
                table[current] = HashNode(key, id);
                size++;
                return;
            }
            //已经存在【LSH的标准是允许一个键对应多个值，这就是lsh中的桶，后续查询要在这个桶中找到所有候选向]量】
            if (table[current].key == key) {
                table[current].ids.push_back(id);
                return;
            }
            
            attempt++;
        }
        
        }
vector<int>find(const string &key)
{
    size_t index =hash_func(key);
    size_t attempt=0;

    while(attempt<log2(capacity)){
        size_t current=probe(index,attempt);

        // 如果遇到真正未占用的槽（非删除的），说明键不存在
            if (!table[current].occupied && !table[current].deleted) {
                return {};
            }
            
            // 如果找到匹配的键且未被删除
            if (table[current].occupied && 
                !table[current].deleted && 
                table[current].key == key) {
                return table[current].ids;
            }
            
            attempt++; // 继续探查
    }
    //探查完所有位置都没找到
    return {};
}


    };

    /*将向量稀疏化存储，只存非零量，这样优化速度*/
struct SparseVector {
    vector<int> indices;   // 非零元素的维度索引
    vector<double> values; // 对应的非零值
};

/*产生投影向量（投影向量是稠密的无法稀疏化）*/

vector<vector<double>> generate_random_vectors(int num_hashes, int dim) {
 
vector<uint32_t> seed_data{42};  
seed_seq seq(seed_data.begin(), seed_data.end());
mt19937 gen(seq);  
 
    normal_distribution<double> dist(0.0, 1.0); // 高斯随机数

    vector<vector<double>> projections(num_hashes, vector<double>(dim));
    for (auto& vec : projections) {
        for (auto& x : vec) {
            x = dist(gen);
        }
    }
    return projections;
    
}

/*通过随机向量计算测试向量数据的Key*/
// 计算向量的SRP哈希码（用bool 存的原因是SRP投影判断向量在投影的哪一边，自然输出就是布尔类型）但是后面要转化为string类型操作
    //vec待测向量
vector<bool> compute_hash(const SparseVector& vec, 
                         const vector<vector<double>>& projections) {
    vector<bool> hash_code(number_hash, false);
    for (int i = 0; i < number_hash; ++i) {
        double dot = 0.0;
        // 只计算非零维度的点积
        for (int j = 0; j < vec.indices.size(); ++j) {
            int dim = vec.indices[j];
            dot += vec.values[j] * projections[i][dim];
        }
        hash_code[i] = (dot >= 0);
    }
    return hash_code;
}



 
using HashTable=MyHashTable;
 
// 构建哈希表
void build_hash_table(
    HashTable& table,
    const vector<SparseVector>& sparse_vectors,  // 接收稀疏向量
    const vector<vector<double>>& projections   //产生的随机向量
) {
    // 遍历所有稀疏向量
    for (int vec_id = 0; vec_id < sparse_vectors.size(); ++vec_id) {
        // 基于投影计算当前向量对应的Key
        auto hash_code = compute_hash(sparse_vectors[vec_id], projections);
        
        //  !要规定哈希码的长度方便比较
        if (hash_code.size() < number_hash) {
            hash_code.resize(number_hash, false);  // 不足补0
        }

        // 将bool哈希码转为string型
        string hash_key;
        for (bool bit : hash_code) {
            hash_key += (bit ? '1' : '0');  
        }

        // 将把这个向量id和key插入哈希表
        table.insert(hash_key, vec_id);  
    }
}

/*数据输入*/


struct Query {
    vector<int> ids;
    vector<double> vals;
};//query


int main() {
    /*检索库的输入*/
    int row, col, nnz, topk;
    cin >> row >> col >> nnz >> topk;
    int num_tables =3;  // 哈希表数量

vector<MyHashTable> hash_tables;  // 初始化哈希表
for(int i=0;i<num_tables;++i){
    hash_tables.emplace_back(1<<number_hash);
}



// 定义投影向量组（每个哈希表对应一组投影向量）
vector<vector<vector<double>>> all_projections(num_tables); 
for (unsigned int i = 0; i < num_tables; ++i) {
    vector<uint32_t> seed_data{i};  // 每个哈希表用不同的种子
    seed_seq seq(seed_data.begin(), seed_data.end());
    mt19937 gen(seq);  
    all_projections[i] = generate_random_vectors(number_hash, col);
}



 
    // 读取indptr数组——几个向量的数据 在 data 和 indices 中的分布
    vector<int> indptr(row + 1);
    for (int i = 0; i <= row; i++) {
        cin >> indptr[i];
    }
    

    // 读取indices数组——每个非零数据在各自向量中的第几位——id
    vector<int> indices(nnz);//nnz是非零向量个数（number of non-zero elements）
    for (int i = 0; i < nnz; i++) {
        cin >> indices[i];
    }
    

    // 读取data数组，每个非零向量的 具体值
    vector<double> data(nnz);
    for (int i = 0; i < nnz; i++) {
        cin >> data[i];
    }

    // 在读取完 data 后构建稀疏向量
vector<SparseVector> sparse_vectors(row);
for (int i = 0; i < row; ++i) {
    for (int j = indptr[i]; j < indptr[i+1]; ++j) {
        sparse_vectors[i].indices.push_back(indices[j]);
        sparse_vectors[i].values.push_back(data[j]);
    }
}

for (int i = 0; i < num_tables; ++i) {
    build_hash_table(hash_tables[i], sparse_vectors, all_projections[i]); // 传入对应组的投影向量
}

 /*询问向量的输入*/
   
   int nq;
cin >> nq;

vector<Query> queries(nq);
// 读取所有查询数据
for (int q = 0; q < nq; q++) {
    int q_nnz;
    cin >> q_nnz;  // 当前查询的非零项数量
    queries[q].ids.resize(q_nnz);
    queries[q].vals.resize(q_nnz);
    for (int i = 0; i < q_nnz; i++) cin >> queries[q].ids[i];   // 读ID
    for (int i = 0; i < q_nnz; i++) cin >> queries[q].vals[i];  // 读对应值
}

// 处理每个查询
for (const auto& query : queries) {
    // 构建稀疏查询向量
    SparseVector query_vec;
    query_vec.indices = query.ids;
    query_vec.values = query.vals;

    //  初始化候选集（使用unordered_set去重）
    unordered_set<int> candidate_ids;
    
    // 遍历所有哈希表寻找候选向量
    for (int table_idx = 0; table_idx < num_tables; ++table_idx) {
        // 计算当前查询的哈希码
        auto hash_code = compute_hash(query_vec, all_projections[table_idx]);
        
        // 将哈希码转为字符串key
        string hash_key;
        for (bool b : hash_code) hash_key += b ? '1' : '0';
        
        // 查找邻近桶（思路是仅有一位数字不同的哈希码的集合）
       bool search_acc=1;
        for (int i = 0; i < hash_key.size(); ++i) {

            string neighbor = hash_key;
            neighbor[i] = neighbor[i] == '1' ? '0' : '1';  // 翻转第i位

            for (int id : hash_tables[table_idx].find(neighbor)) {
                candidate_ids.insert(id);
                 if (candidate_ids.size() >= 2 * topk) {
                    search_acc=0;
                break;
           
        }
            }
            if(search_acc==0)break;//加入限制条件：在找到向量数量足够多的时候终止进程跳出循环
        }
        
        // 匹配
        for (int id : hash_tables[table_idx].find(hash_key)) {
            candidate_ids.insert(id);
        }
        
        // 提前终止：候选集足够大时不再查后续表
        if (candidate_ids.size() >= 2 * topk) break;
    }

    // 计算候选向量与查询的相似度得分
    vector<pair<double, int>> scores;
    scores.reserve(candidate_ids.size());  // 预分配内存
    
    // 建立查询向量的快速查找表（优化内积计算）
    unordered_map<int, double> query_map;
    for (int i = 0; i < query.ids.size(); ++i) {
        query_map[query.ids[i]] = query.vals[i];
    }
    
    // 遍历所有候选向量
    for (int id : candidate_ids) {
        double sum = 0.0;
        const auto& vec = sparse_vectors[id];  // 获取候选向量
        
        //稀疏内积计算：只遍历候选向量的非零维度
        for (int j = 0; j < vec.indices.size(); ++j) {
            int dim = vec.indices[j];
            if (query_map.count(dim)) {  // 只在查询也非零的维度计算
                sum += query_map[dim] * vec.values[j];
            }
        }
        
        //  只保留正相关结果（可选优化）
        if (sum > 0) scores.emplace_back(sum, id);
    }

   // 排序
sort(scores.begin(), scores.end(), [](const pair<double, int>& a, const pair<double, int>& b) {
    if (a.first != b.first) return a.first > b.first;  // 内积大的在前
    return a.second < b.second;  // 内积相同则id小的在前
});

// 输出topk个
int output_k = min(topk, (int)scores.size());
for (int i = 0; i < output_k; ++i) {
    cout << scores[i].second;
    if (i < output_k - 1) cout << " ";
}
cout << endl;

}


}    
