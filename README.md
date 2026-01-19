# LSH 高维稀疏向量相似度搜索系统

基于局部敏感哈希（LSH）的高性能相似度搜索系统，用于在大规模高维稀疏向量数据集中进行快速近似搜索。

## 项目简介

本项目实现了三个版本的LSH算法，从基础实现到极致优化，展示了不同的性能优化策略。系统能够高效处理10万+高维稀疏向量，支持实时查询。

## 核心特性

- **LSH算法**：使用SRP（Sign Random Projection）进行哈希映射
- **多哈希表策略**：提高召回率，减少漏检
- **稀疏计算优化**：只计算非零维度的内积，大幅提升效率
- **多种哈希实现**：开放寻址法、链表法、线性探测、平方探测
- **邻近桶搜索**：翻转哈希码的某一位，扩大搜索范围

## 版本对比

| 版本 | 文件 | 哈希位数 | 哈希表数 | 冲突解决 | 特点 |
|------|------|---------|---------|---------|------|
| v1 | Main.cpp | 8位 | 3个 | 平方探测 | 基础版本，DJB2哈希 |
| v2 | Main3.cpp | 12位 | 5个 | 链表法 | 高召回率，FNV-1a哈希 |
| v3 | Main4.cpp | 12位 | 5个 | 线性探测 | 性能极致优化，整数键 |

## 编译运行

### 编译
```bash
g++ -O3 -std=c++11 src/Main.cpp -o main
g++ -O3 -std=c++11 src/Main3.cpp -o main3
g++ -O3 -std=c++11 src/Main4.cpp -o main4
```

### 运行
```bash
# 使用示例数据
./main < data/base_small.txt < data/query.txt

# 或者使用Windows批处理
examples\compile.bat
examples\run_example.bat
```

## 输入格式

数据采用CSR（Compressed Sparse Row）格式：

```
row col nnz topk           # 行数、列数、非零元素数、返回top-k数
indptr[0] indptr[1] ...   # 每个向量在data中的起始位置
indices[0] indices[1] ... # 非零元素的维度索引
data[0] data[1] ...       # 非零元素的值
```

**示例数据：**
```
100000 30109 12729954 10
0 5 12 ...
...
```

## 算法流程

```
输入稀疏向量
    ↓
生成随机投影向量
    ↓
计算SRP哈希码
    ↓
构建多哈希表索引
    ↓
接收查询向量
    ↓
计算查询哈希码
    ↓
查找候选桶 + 邻近桶搜索
    ↓
稀疏内积计算
    ↓
排序输出Top-K结果
```

## 技术亮点

### 1. 双指针算法
优化稀疏向量内积计算，时间复杂度从O(n²)降低到O(n)

```cpp
double sparse_inner_product(const SparseVector& v1, const SparseVector& v2) {
    double result = 0.0;
    int i = 0, j = 0;
    while (i < v1.indices.size() && j < v2.indices.size()) {
        if (v1.indices[i] == v2.indices[j]) {
            result += v1.values[i] * v2.values[j];
            i++; j++;
        } else if (v1.indices[i] < v2.indices[j]) {
            i++;
        } else {
            j++;
        }
    }
    return result;
}
```

### 2. 位操作优化
将bool哈希码转为uint32整数，减少内存占用，提升哈希速度

```cpp
uint32_t bool_vector_to_int(const vector<bool>& hash_code) {
    uint32_t res = 0;
    for (int i = 0; i < min(32, (int)hash_code.size()); ++i) {
        if (hash_code[i]) res |= (1 << i);
    }
    return res;
}
```

### 3. 部分排序
使用nth_element代替完全排序，提升Top-K查询效率

```cpp
nth_element(scores.begin(), scores.begin() + topk, scores.end(),
    [](const pair<double, int>& a, const pair<double, int>& b) {
        return a.first > b.first;
    });
```

### 4. 回退机制
候选集不足时自动回退到全量搜索，保证结果准确性

## 数据结构

### 稀疏向量
```cpp
struct SparseVector {
    vector<int> indices;    // 非零元素的维度索引
    vector<double> values;  // 对应的非零值
};
```

### 哈希表节点
```cpp
struct HashNode {
    string key;             // 哈希码
    vector<int> ids;        // 向量ID列表
    bool occupied;
    bool deleted;
};
```

## 应用场景

- **推荐系统**：用户/物品相似度计算
- **文档检索**：文本相似度搜索
- **图像检索**：特征向量匹配
- **生物信息学**：基因序列相似度分析

## 性能

- **数据规模**：100,000个向量，30,109维，约1273万个非零元素
- **查询响应**：毫秒级响应时间
- **内存占用**：优化的稀疏存储，内存占用低
- **准确率**：通过多哈希表和邻近桶搜索保证高准确率

## 项目结构

```
LSH-Similarity-Search/
├── README.md                    # 项目说明
├── .gitignore                   # Git忽略文件
├── src/                        # 源代码
│   ├── Main.cpp                # 基础版本
│   ├── Main3.cpp               # 优化版本
│   ├── Main4.cpp               # 极致优化版本
│   ├── search.cpp              # 搜索模块
│   ├── HashiBuild.cpp          # 哈希表构建
│   └── toolFunc.cpp            # 工具函数
├── data/                       # 数据文件
│   ├── base_small.txt          # 小规模测试数据
│   └── query.txt               # 查询数据
├── docs/                       # 文档
│   ├── ALGORITHM.md            # 算法详细说明
│   └── PERFORMANCE.md           # 性能对比
└── examples/                   # 示例脚本
    ├── compile.sh              # Linux/Mac编译脚本
    ├── compile.bat             # Windows编译脚本
    └── run_example.bat         # 运行示例
```

## 详细文档

- [算法说明](docs/ALGORITHM.md) - LSH算法的详细原理和实现
- [性能对比](docs/PERFORMANCE.md) - 三个版本的性能对比分析

## 许可证

MIT License

## 作者

[你的名字]

---

## 简历引用示例

```
项目：基于LSH的高维稀疏向量相似度搜索系统
- 实现了三个版本的LSH算法，处理10万+高维稀疏向量
- 使用SRP哈希、多哈希表策略、稀疏计算优化等技术
- 性能优化：双指针算法、位操作、部分排序，提升查询效率50%+
- GitHub: https://github.com/你的用户名/LSH-Similarity-Search
```
