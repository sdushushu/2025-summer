# 2025-summer
project1

1.优化SM4的软件执行效率，首先对于朴素的SM4算法，可以从代码层面进行优化。通过使用T表，将S盒替换和现行变换两个步骤合并成查T表一个步骤，可以有效减少内存的使用，从而提高算法执行效率。
2.SIMD并行优化也可以优化SM4的执行效率，使用AVX2一次并行 8 个 32 位数据，Gather 指令可以在 1 条指令里按索引从 T-table 里取多个值。
3.SM4 的线性变换 L 全是 32 位循环左移的 XOR 组合，AVX-512 有 VPROLD（rotate-left 32-bit）可一条指令完成 _mm512_rol_epi32，减少两条移位一条或的组合，可以大幅提提高吞吐率。 

project4

1.使用pad()函数来进行初始化
2.使用compress()函数来进行消息分块以及64纶迭代
3.关于SM3的优化代码在OptimizedSM3类中
4.使用length_extension_attack()函数来进行长度攻击
5.使用MerkleTree函数来实现构建Merkle树
6.使用verify函数来验证存在性

project5

1.使用mont_reduce函数来运用预计算表法，以实现大数模约减优化
2.使用point_add函数来实现固定点点乘
3.使用naf_encode函数来实现非固定点点乘优化
