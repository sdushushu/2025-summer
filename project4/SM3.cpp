#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <stdexcept>


using namespace std;

// ============================== SM3 基础实现 ==============================
class SM3 {
public:
    static const size_t BLOCK_SIZE = 64;
    static const size_t DIGEST_SIZE = 32;

    SM3() { reset(); }

    void reset() {
        state[0] = 0x7380166F;
        state[1] = 0x4914B2B9;
        state[2] = 0x172442D7;
        state[3] = 0xDA8A0600;
        state[4] = 0xA96F30BC;
        state[5] = 0x163138AA;
        state[6] = 0xE38DEE4D;
        state[7] = 0xB0FB0E4E;
        count = 0;
        buffer.clear();
    }

    void update(const uint8_t* data, size_t len) {
        size_t offset = 0;
        count += len * 8;  // 按比特计数

        // 处理缓冲区中的部分数据
        if (!buffer.empty()) {
            size_t to_copy = min(BLOCK_SIZE - buffer.size(), len);
            buffer.insert(buffer.end(), data, data + to_copy);
            offset += to_copy;

            if (buffer.size() == BLOCK_SIZE) {
                compress(buffer.data());
                buffer.clear();
            }
        }

        // 处理完整块
        while (offset + BLOCK_SIZE <= len) {
            compress(data + offset);
            offset += BLOCK_SIZE;
        }

        // 保存剩余数据到缓冲区
        if (offset < len) {
            buffer.insert(buffer.end(), data + offset, data + len);
        }
    }

    vector<uint8_t> digest() {
        vector<uint8_t> result(DIGEST_SIZE);
        uint32_t saved_state[8];
        memcpy(saved_state, state, sizeof(state));
        vector<uint8_t> saved_buffer = buffer;
        uint64_t saved_count = count;

        pad();

        if (!buffer.empty()) {
            compress(buffer.data());
        }

        for (size_t i = 0; i < 8; i++) {
            write_uint32_be(result.data() + i * 4, state[i]);
        }

        memcpy(state, saved_state, sizeof(state));
        buffer = saved_buffer;
        count = saved_count;

        return result;
    }

private:
    uint32_t state[8];
    uint64_t count;
    vector<uint8_t> buffer;

    static inline uint32_t left_rotate(uint32_t x, uint32_t n) {
        return (x << n) | (x >> (32 - n));
    }

    static inline uint32_t ff0(uint32_t x, uint32_t y, uint32_t z) {
        return x ^ y ^ z;
    }

    static inline uint32_t ff1(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) | (x & z) | (y & z);
    }

    static inline uint32_t gg0(uint32_t x, uint32_t y, uint32_t z) {
        return x ^ y ^ z;
    }

    static inline uint32_t gg1(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) | (~x & z);
    }

    static inline uint32_t p0(uint32_t x) {
        return x ^ left_rotate(x, 9) ^ left_rotate(x, 17);
    }

    static inline uint32_t p1(uint32_t x) {
        return x ^ left_rotate(x, 15) ^ left_rotate(x, 23);
    }

    void write_uint32_be(uint8_t* out, uint32_t val) {
        out[0] = (val >> 24) & 0xFF;
        out[1] = (val >> 16) & 0xFF;
        out[2] = (val >> 8) & 0xFF;
        out[3] = val & 0xFF;
    }

    uint32_t read_uint32_be(const uint8_t* in) {
        return (in[0] << 24) | (in[1] << 16) | (in[2] << 8) | in[3];
    }

    void pad() {
        size_t len = buffer.size();
        uint64_t bit_count = count;

        // 添加0x80
        buffer.push_back(0x80);

        // 填充0直到长度满足 
        size_t pad_len = (len < 56) ? (56 - len) : (120 - len);
        buffer.insert(buffer.end(), pad_len, 0);

        // 添加比特长度（大端）
        for (int i = 7; i >= 0; i--) {
            buffer.push_back((bit_count >> (i * 8)) & 0xFF);
        }
    }

    void compress(const uint8_t* block) {
        uint32_t w[68];
        uint32_t w_prime[64];

        // 消息扩展
        for (int i = 0; i < 16; i++) {
            w[i] = read_uint32_be(block + i * 4);
        }

        for (int i = 16; i < 68; i++) {
            w[i] = p1(w[i - 16] ^ w[i - 9] ^ left_rotate(w[i - 3], 15))
                ^ left_rotate(w[i - 13], 7)
                ^ w[i - 6];
        }

        for (int i = 0; i < 64; i++) {
            w_prime[i] = w[i] ^ w[i + 4];
        }

        // 压缩函数
        uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

        for (int j = 0; j < 64; j++) {
            uint32_t ss1, ss2, tt1, tt2;

            if (j < 16) {
                ss1 = left_rotate(left_rotate(a, 12) + e + left_rotate(0x79CC4519, j), 7);
                ss2 = ss1 ^ left_rotate(a, 12);
                tt1 = ff0(a, b, c) + d + ss2 + w_prime[j];
                tt2 = gg0(e, f, g) + h + ss1 + w[j];
            }
            else {
                ss1 = left_rotate(left_rotate(a, 12) + e + left_rotate(0x7A879D8A, j), 7);
                ss2 = ss1 ^ left_rotate(a, 12);
                tt1 = ff1(a, b, c) + d + ss2 + w_prime[j];
                tt2 = gg1(e, f, g) + h + ss1 + w[j];
            }

            d = c;
            c = left_rotate(b, 9);
            b = a;
            a = tt1;
            h = g;
            g = left_rotate(f, 19);
            f = e;
            e = p0(tt2);
        }

        state[0] ^= a;
        state[1] ^= b;
        state[2] ^= c;
        state[3] ^= d;
        state[4] ^= e;
        state[5] ^= f;
        state[6] ^= g;
        state[7] ^= h;
    }
};

// ============================== 优化版SM3 (4轮展开) ==============================
class OptimizedSM3 {
public:
    static const size_t BLOCK_SIZE = 64;
    static const size_t DIGEST_SIZE = 32;

    OptimizedSM3() { reset(); }

    void reset() {
        state[0] = 0x7380166F;
        state[1] = 0x4914B2B9;
        state[2] = 0x172442D7;
        state[3] = 0xDA8A0600;
        state[4] = 0xA96F30BC;
        state[5] = 0x163138AA;
        state[6] = 0xE38DEE4D;
        state[7] = 0xB0FB0E4E;
        count = 0;
        buffer.clear();
    }

    // 添加公共方法用于设置内部状态
    void set_state(const uint8_t* new_state) {
        for (int i = 0; i < 8; i++) {
            state[i] = (new_state[i * 4] << 24) |
                (new_state[i * 4 + 1] << 16) |
                (new_state[i * 4 + 2] << 8) |
                new_state[i * 4 + 3];
        }
    }

    // 添加公共方法用于设置消息计数
    void set_count(uint64_t new_count) {
        count = new_count;
    }

    void update(const uint8_t* data, size_t len) {
        size_t offset = 0;
        count += len * 8;  

        // 处理缓冲区中的部分数据
        if (!buffer.empty()) {
            size_t to_copy = min(BLOCK_SIZE - buffer.size(), len);
            buffer.insert(buffer.end(), data, data + to_copy);
            offset += to_copy;

            if (buffer.size() == BLOCK_SIZE) {
                compress(buffer.data());
                buffer.clear();
            }
        }

        // 处理完整块
        while (offset + BLOCK_SIZE <= len) {
            compress(data + offset);
            offset += BLOCK_SIZE;
        }

        // 保存剩余数据到缓冲区
        if (offset < len) {
            buffer.insert(buffer.end(), data + offset, data + len);
        }
    }

    vector<uint8_t> digest() {
        vector<uint8_t> result(DIGEST_SIZE);
        uint32_t saved_state[8];
        memcpy(saved_state, state, sizeof(state));
        vector<uint8_t> saved_buffer = buffer;
        uint64_t saved_count = count;
        pad();
        if (!buffer.empty()) {
            compress(buffer.data());
        }
        
        for (size_t i = 0; i < 8; i++) {
            write_uint32_be(result.data() + i * 4, state[i]);
        }


        memcpy(state, saved_state, sizeof(state));
        buffer = saved_buffer;
        count = saved_count;

        return result;
    }

private:
    uint32_t state[8];
    uint64_t count;
    vector<uint8_t> buffer;

    static inline uint32_t left_rotate(uint32_t x, uint32_t n) {
        return (x << n) | (x >> (32 - n));
    }

    static inline uint32_t ff0(uint32_t x, uint32_t y, uint32_t z) {
        return x ^ y ^ z;
    }

    static inline uint32_t ff1(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) | (x & z) | (y & z);
    }

    static inline uint32_t gg0(uint32_t x, uint32_t y, uint32_t z) {
        return x ^ y ^ z;
    }

    static inline uint32_t gg1(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) | (~x & z);
    }

    static inline uint32_t p0(uint32_t x) {
        return x ^ left_rotate(x, 9) ^ left_rotate(x, 17);
    }

    static inline uint32_t p1(uint32_t x) {
        return x ^ left_rotate(x, 15) ^ left_rotate(x, 23);
    }

    void write_uint32_be(uint8_t* out, uint32_t val) {
        out[0] = (val >> 24) & 0xFF;
        out[1] = (val >> 16) & 0xFF;
        out[2] = (val >> 8) & 0xFF;
        out[3] = val & 0xFF;
    }

    uint32_t read_uint32_be(const uint8_t* in) {
        return (in[0] << 24) | (in[1] << 16) | (in[2] << 8) | in[3];
    }

    void pad() {
        size_t len = buffer.size();
        uint64_t bit_count = count;

        // 添加0x80
        buffer.push_back(0x80);

        // 填充0直到长度满足
        size_t pad_len = (len < 56) ? (56 - len) : (120 - len);
        buffer.insert(buffer.end(), pad_len, 0);

        // 添加比特长度（大端）
        for (int i = 7; i >= 0; i--) {
            buffer.push_back((bit_count >> (i * 8)) & 0xFF);
        }
    }

    // 4轮展开的压缩函数
    void compress(const uint8_t* block) {
        uint32_t w[68];
        uint32_t w_prime[64];

        // 消息扩展
        for (int i = 0; i < 16; i++) {
            w[i] = read_uint32_be(block + i * 4);
        }

        for (int i = 16; i < 68; i++) {
            w[i] = p1(w[i - 16] ^ w[i - 9] ^ left_rotate(w[i - 3], 15))
                ^ left_rotate(w[i - 13], 7)
                ^ w[i - 6];
        }

        for (int i = 0; i < 64; i++) {
            w_prime[i] = w[i] ^ w[i + 4];
        }

        // 寄存器分配
        uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

        // 4轮展开处理
        for (int j = 0; j < 64; j += 4) {
            // 第1轮
            uint32_t ss1 = left_rotate(left_rotate(a, 12) + e + left_rotate(j < 16 ? 0x79CC4519 : 0x7A879D8A, j), 7);
            uint32_t ss2 = ss1 ^ left_rotate(a, 12);
            uint32_t tt1 = (j < 16 ? ff0(a, b, c) : ff1(a, b, c)) + d + ss2 + w_prime[j];
            uint32_t tt2 = (j < 16 ? gg0(e, f, g) : gg1(e, f, g)) + h + ss1 + w[j];

            uint32_t next_d = c;
            uint32_t next_c = left_rotate(b, 9);
            uint32_t next_b = a;
            uint32_t next_a = tt1;
            uint32_t next_h = g;
            uint32_t next_g = left_rotate(f, 19);
            uint32_t next_f = e;
            uint32_t next_e = p0(tt2);

            // 第2轮 (使用更新后的寄存器)
            ss1 = left_rotate(left_rotate(next_a, 12) + next_e + left_rotate(j + 1 < 16 ? 0x79CC4519 : 0x7A879D8A, j + 1), 7);
            ss2 = ss1 ^ left_rotate(next_a, 12);
            tt1 = (j + 1 < 16 ? ff0(next_a, next_b, next_c) : ff1(next_a, next_b, next_c)) + next_d + ss2 + w_prime[j + 1];
            tt2 = (j + 1 < 16 ? gg0(next_e, next_f, next_g) : gg1(next_e, next_f, next_g)) + next_h + ss1 + w[j + 1];

            d = next_c;
            c = left_rotate(next_b, 9);
            b = next_a;
            a = tt1;
            h = next_g;
            g = left_rotate(next_f, 19);
            f = next_e;
            e = p0(tt2);

            // 第3轮
            ss1 = left_rotate(left_rotate(a, 12) + e + left_rotate(j + 2 < 16 ? 0x79CC4519 : 0x7A879D8A, j + 2), 7);
            ss2 = ss1 ^ left_rotate(a, 12);
            tt1 = (j + 2 < 16 ? ff0(a, b, c) : ff1(a, b, c)) + d + ss2 + w_prime[j + 2];
            tt2 = (j + 2 < 16 ? gg0(e, f, g) : gg1(e, f, g)) + h + ss1 + w[j + 2];

            next_d = c;
            next_c = left_rotate(b, 9);
            next_b = a;
            next_a = tt1;
            next_h = g;
            next_g = left_rotate(f, 19);
            next_f = e;
            next_e = p0(tt2);

            // 第4轮
            ss1 = left_rotate(left_rotate(next_a, 12) + next_e + left_rotate(j + 3 < 16 ? 0x79CC4519 : 0x7A879D8A, j + 3), 7);
            ss2 = ss1 ^ left_rotate(next_a, 12);
            tt1 = (j + 3 < 16 ? ff0(next_a, next_b, next_c) : ff1(next_a, next_b, next_c)) + next_d + ss2 + w_prime[j + 3];
            tt2 = (j + 3 < 16 ? gg0(next_e, next_f, next_g) : gg1(next_e, next_f, next_g)) + next_h + ss1 + w[j + 3];

            d = next_c;
            c = left_rotate(next_b, 9);
            b = next_a;
            a = tt1;
            h = next_g;
            g = left_rotate(next_f, 19);
            f = next_e;
            e = p0(tt2);
        }

        state[0] ^= a;
        state[1] ^= b;
        state[2] ^= c;
        state[3] ^= d;
        state[4] ^= e;
        state[5] ^= f;
        state[6] ^= g;
        state[7] ^= h;
    }
};

// ============================== 长度扩展攻击 ==============================
vector<uint8_t> length_extension_attack(
    const vector<uint8_t>& original_hash,
    size_t original_len,
    const vector<uint8_t>& extension
) {
    // 从原始哈希恢复内部状态
    if (original_hash.size() != 32)
        throw invalid_argument("Invalid hash length");

    OptimizedSM3 sm3;
    sm3.set_state(original_hash.data());

    // 计算原始消息填充后的总比特数
    size_t pad_len = (55 - (original_len % 64) + 64) % 64;
    uint64_t total_bits_after_padding = (original_len + 1 + pad_len + 8) * 8;

    // 设置正确的消息计数（包含填充和扩展）
    sm3.set_count(total_bits_after_padding + extension.size() * 8);

    // 处理扩展数据
    sm3.update(extension.data(), extension.size());
    return sm3.digest();
}

// ============================== Merkle树实现 (RFC6962) ==============================
class MerkleTree {
public:
    MerkleTree(const vector<vector<uint8_t>>& leaves) {
        if (leaves.empty()) return;

        // 确保叶子数量是2的幂
        size_t n = leaves.size();
        size_t tree_size = 1;
        while (tree_size < n) tree_size <<= 1;

        tree.resize(2 * tree_size);
        for (size_t i = 0; i < n; i++) {
            tree[tree_size + i] = hash_leaf(leaves[i]);
        }
        for (size_t i = n; i < tree_size; i++) {
            tree[tree_size + i] = vector<uint8_t>(32, 0);
        }

        // 构建树
        for (size_t i = tree_size - 1; i > 0; i--) {
            tree[i] = hash_node(tree[2 * i], tree[2 * i + 1]);
        }
    }

    vector<uint8_t> root() const {
        return tree.empty() ? vector<uint8_t>(32, 0) : tree[1];
    }

    // 存在性证明
    vector<vector<uint8_t>> proof(size_t index) const {
        vector<vector<uint8_t>> proof;
        if (tree.empty()) return proof;

        size_t n = tree.size() / 2;
        if (index >= n) throw out_of_range("Index out of range");

        size_t pos = n + index;

        while (pos > 1) {
            proof.push_back(tree[pos ^ 1]);
            pos /= 2;
        }

        return proof;
    }

    // 验证存在性证明
    static bool verify(
        const vector<uint8_t>& leaf,
        const vector<uint8_t>& root,
        size_t index,
        size_t tree_size,
        const vector<vector<uint8_t>>& proof
    ) {
        vector<uint8_t> current = hash_leaf(leaf);
        size_t n = 1;
        while (n < tree_size) n <<= 1;
        size_t pos = n + index;

        for (const auto& p : proof) {
            if (pos % 2 == 1) {
                current = hash_node(p, current);
            }
            else {
                current = hash_node(current, p);
            }
            pos /= 2;
        }

        return current == root;
    }

    // 生成不存在性证明 
    pair<vector<vector<uint8_t>>, vector<vector<uint8_t>>> absence_proof(
        const vector<uint8_t>& target,
        size_t& insert_pos
    ) const {
        if (tree.empty()) {
            insert_pos = 0;
            return { {}, {} };
        }

        // 在叶子节点中定位插入位置
        size_t n = tree.size() / 2;
        size_t low = 0, high = n;
        insert_pos = n; 

        while (low < high) {
            size_t mid = (low + high) / 2;
            int cmp = memcmp(target.data(), tree[n + mid].data(), 32);
            if (cmp == 0) {
                throw runtime_error("Target exists in tree");
            }
            else if (cmp < 0) {
                high = mid;
                insert_pos = mid;
            }
            else {
                low = mid + 1;
            }
        }

        // 处理边界情况
        vector<vector<uint8_t>> predecessor_proof;
        vector<vector<uint8_t>> successor_proof;

        if (insert_pos > 0) {
            predecessor_proof = proof(insert_pos - 1);
        }

        if (insert_pos < n) {
            successor_proof = proof(insert_pos);
        }

        return { predecessor_proof, successor_proof };
    }

private:
    vector<vector<uint8_t>> tree;

    static vector<uint8_t> hash_leaf(const vector<uint8_t>& data) {
        OptimizedSM3 sm3;
        sm3.update(data.data(), data.size());
        return sm3.digest();
    }

    static vector<uint8_t> hash_node(const vector<uint8_t>& left, const vector<uint8_t>& right) {
        OptimizedSM3 sm3;
        sm3.update(left.data(), left.size());
        sm3.update(right.data(), right.size());
        return sm3.digest();
    }
};

// ============================== 辅助函数 ==============================
string hex_str(const vector<uint8_t>& data) {
    static const char* hex_digits = "0123456789abcdef";
    string result;
    result.reserve(data.size() * 2);

    for (uint8_t b : data) {
        result.push_back(hex_digits[b >> 4]);
        result.push_back(hex_digits[b & 0x0F]);
    }

    return result;
}

vector<uint8_t> str_to_vec(const string& s) {
    return vector<uint8_t>(s.begin(), s.end());
}

// ============================== 测试函数 ==============================
void test_sm3() {
    SM3 sm3;
    string test_str = "abc";
    sm3.update((const uint8_t*)test_str.data(), test_str.size());
    auto digest = sm3.digest();

    string result = hex_str(digest);
    string expected = "66c7f0f462eeedd9d1f2d46bdc10e4e24167c4875cf2f7a2297da02b8f4ba8e0";

    cout << "SM3(\"abc\") = " << result << endl;
    cout << "Expected   = " << expected << endl;
    cout << "Test " << (result == expected ? "PASSED" : "FAILED") << endl;
}

void test_optimized_sm3() {
    OptimizedSM3 sm3;
    string test_str = "abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcd";
    sm3.update((const uint8_t*)test_str.data(), test_str.size());
    auto digest = sm3.digest();

    string result = hex_str(digest);
    string expected = "debe9ff92275b8a138604889c18e5a4d6fdb70e5387e5765293dcba39c0c5732";

    cout << "Optimized SM3(long) = " << result << endl;
    cout << "Expected           = " << expected << endl;
    cout << "Test " << (result == expected ? "PASSED" : "FAILED") << endl;
}

void test_length_extension() {
    string secret = "secret";
    string original_msg = "data";
    string extension = "append";

    // 原始消息的哈希
    OptimizedSM3 sm3;
    sm3.update((const uint8_t*)secret.data(), secret.size());
    sm3.update((const uint8_t*)original_msg.data(), original_msg.size());
    auto original_hash = sm3.digest();

    // 长度扩展攻击
    size_t total_len = secret.size() + original_msg.size();
    auto forged_hash = length_extension_attack(
        original_hash,
        total_len,
        str_to_vec(extension)
    );

    // 验证攻击结果
    sm3.reset();
    sm3.update((const uint8_t*)secret.data(), secret.size());
    sm3.update((const uint8_t*)original_msg.data(), original_msg.size());

    // 添加填充后附加扩展
    vector<uint8_t> padded_msg = str_to_vec(original_msg);
    size_t pad_len = (55 - (total_len % 64) + 64) % 64;
    padded_msg.push_back(0x80);
    padded_msg.insert(padded_msg.end(), pad_len, 0);
    uint64_t bit_len = total_len * 8;
    for (int i = 7; i >= 0; i--) {
        padded_msg.push_back((bit_len >> (i * 8)) & 0xFF);
    }
    padded_msg.insert(padded_msg.end(), extension.begin(), extension.end());

    sm3.update(padded_msg.data() + secret.size(), padded_msg.size() - secret.size());
    auto real_hash = sm3.digest();

    cout << "Length Extension Attack: "
        << (forged_hash == real_hash ? "SUCCESS" : "FAILED")
        << endl;
}

void test_merkle_tree() {
    // 生成10万叶子节点
    vector<vector<uint8_t>> leaves;
    for (int i = 0; i < 100000; i++) {
        leaves.push_back(str_to_vec("leaf" + to_string(i)));
    }

    // 构建Merkle树
    MerkleTree tree(leaves);
    auto root = tree.root();
    cout << "Merkle Root: " << hex_str(root).substr(0, 16) << "..." << endl;

    // 存在性证明
    size_t index = 12345;
    auto proof = tree.proof(index);
    bool valid = MerkleTree::verify(
        leaves[index], root, index, leaves.size(), proof
    );
    cout << "Existence Proof: " << (valid ? "VALID" : "INVALID") << endl;

    // 不存在性证明
    try {
        size_t insert_pos;
        auto absence_proof = tree.absence_proof(str_to_vec("leaf12345"), insert_pos);
        cout << "Absence Proof generated for non-existing node" << endl;
    }
    catch (...) {
        cout << "Absence Proof failed (as expected for existing node)" << endl;
    }

    // 真正不存在的节点
    try {
        size_t insert_pos;
        auto absence_proof = tree.absence_proof(str_to_vec("non_existent_leaf"), insert_pos);
        cout << "Absence Proof generated at position: " << insert_pos << endl;

        // 验证前驱证明
        if (insert_pos > 0) {
            bool pred_valid = MerkleTree::verify(
                leaves[insert_pos - 1],
                root,
                insert_pos - 1,
                leaves.size(),
                absence_proof.first
            );
            cout << "Predecessor Proof: " << (pred_valid ? "VALID" : "INVALID") << endl;
        }

        // 验证后继证明
        if (insert_pos < leaves.size()) {
            bool succ_valid = MerkleTree::verify(
                leaves[insert_pos],
                root,
                insert_pos,
                leaves.size(),
                absence_proof.second
            );
            cout << "Successor Proof: " << (succ_valid ? "VALID" : "INVALID") << endl;
        }
    }
    catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }
}

// ============================== 主函数 ==============================
int main() {
    cout << "===== Basic SM3 Test =====" << endl;
    test_sm3();

    cout << "\n===== Optimized SM3 Test =====" << endl;
    test_optimized_sm3();

    cout << "\n===== Length Extension Attack Test =====" << endl;
    test_length_extension();

    cout << "\n===== Merkle Tree Test (10k leaves) =====" << endl;
    test_merkle_tree();

    return 0;
}
