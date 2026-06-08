# 数据结构与算法

笔试/机试高频考点，C++ 解法。

## 复杂度速查

| 算法 | 时间复杂度 | 空间复杂度 |
|------|-----------|-----------|
| 二分查找 | O(log N) | O(1) |
| 快速排序 | O(N log N) 均摊 | O(log N) |
| 归并排序 | O(N log N) | O(N) |
| BFS / DFS | O(V + E) | O(V) |
| 动态规划（常见）| O(N²) | O(N) |
| 哈希表查找 | O(1) 均摊 | O(N) |

---

## 二分查找

边界条件是最容易出错的地方，记住一套模板。

```cpp
// 找第一个 >= target 的位置（lower_bound）
int lower_bound(vector<int>& nums, int target) {
    int lo = 0, hi = nums.size();   // 注意：hi = size，不是 size-1
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;  // 防止溢出
        if (nums[mid] < target)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo;  // 返回插入位置，lo == hi
}

// 精确查找
int binary_search(vector<int>& nums, int target) {
    int lo = 0, hi = nums.size() - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if      (nums[mid] == target) return mid;
        else if (nums[mid] <  target) lo = mid + 1;
        else                          hi = mid - 1;
    }
    return -1;
}
```

---

## 链表

```cpp
struct ListNode { int val; ListNode* next; };

// 反转链表（高频）
ListNode* reverse(ListNode* head) {
    ListNode* prev = nullptr;
    ListNode* curr = head;
    while (curr) {
        ListNode* next = curr->next;
        curr->next = prev;
        prev = curr;
        curr = next;
    }
    return prev;
}

// 快慢指针：找中点 / 判断环
ListNode* find_middle(ListNode* head) {
    ListNode* slow = head, *fast = head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }
    return slow;  // slow 就是中点
}

bool has_cycle(ListNode* head) {
    ListNode* slow = head, *fast = head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) return true;
    }
    return false;
}
```

---

## 树

```cpp
struct TreeNode { int val; TreeNode* left; TreeNode* right; };

// DFS 三种遍历（递归）
void preorder(TreeNode* root, vector<int>& res) {   // 前序：根左右
    if (!root) return;
    res.push_back(root->val);
    preorder(root->left, res);
    preorder(root->right, res);
}

// BFS 层序遍历（队列）
vector<vector<int>> levelOrder(TreeNode* root) {
    if (!root) return {};
    vector<vector<int>> res;
    queue<TreeNode*> q;
    q.push(root);
    while (!q.empty()) {
        int sz = q.size();           // 当前层的节点数
        res.push_back({});
        for (int i = 0; i < sz; i++) {
            auto node = q.front(); q.pop();
            res.back().push_back(node->val);
            if (node->left)  q.push(node->left);
            if (node->right) q.push(node->right);
        }
    }
    return res;
}

// 树的最大深度
int maxDepth(TreeNode* root) {
    if (!root) return 0;
    return 1 + max(maxDepth(root->left), maxDepth(root->right));
}
```

---

## 图：BFS / DFS

```cpp
// 邻接表表示
vector<vector<int>> graph(n);
graph[u].push_back(v);

// BFS：最短路径（无权图）
vector<int> bfs(int start, int n) {
    vector<int> dist(n, -1);
    queue<int> q;
    dist[start] = 0;
    q.push(start);
    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (int v : graph[u]) {
            if (dist[v] == -1) {
                dist[v] = dist[u] + 1;
                q.push(v);
            }
        }
    }
    return dist;
}

// DFS：连通分量 / 拓扑排序
vector<bool> visited(n, false);

void dfs(int u) {
    visited[u] = true;
    for (int v : graph[u]) {
        if (!visited[v]) dfs(v);
    }
}
```

---

## 动态规划

核心：定义状态 → 找状态转移方程 → 确定边界。

```cpp
// 经典：最长公共子序列（LCS）
int lcs(string& a, string& b) {
    int m = a.size(), n = b.size();
    vector<vector<int>> dp(m+1, vector<int>(n+1, 0));
    for (int i = 1; i <= m; i++)
        for (int j = 1; j <= n; j++)
            if (a[i-1] == b[j-1])
                dp[i][j] = dp[i-1][j-1] + 1;
            else
                dp[i][j] = max(dp[i-1][j], dp[i][j-1]);
    return dp[m][n];
}

// 背包问题（0-1 背包）
int knapsack(vector<int>& w, vector<int>& v, int cap) {
    int n = w.size();
    vector<int> dp(cap+1, 0);
    for (int i = 0; i < n; i++)
        for (int j = cap; j >= w[i]; j--)  // 逆序防止重复选
            dp[j] = max(dp[j], dp[j - w[i]] + v[i]);
    return dp[cap];
}

// 最长递增子序列（LIS）
int lis(vector<int>& nums) {
    vector<int> tails;  // tails[i]：长度为 i+1 的 LIS 的最小末尾元素
    for (int x : nums) {
        auto it = lower_bound(tails.begin(), tails.end(), x);
        if (it == tails.end()) tails.push_back(x);
        else *it = x;
    }
    return tails.size();
}
```

---

## 滑动窗口

```cpp
// 无重复字符的最长子串
int lengthOfLongestSubstring(string s) {
    unordered_map<char, int> last;  // 字符最后出现位置
    int res = 0, left = 0;
    for (int right = 0; right < s.size(); right++) {
        if (last.count(s[right]))
            left = max(left, last[s[right]] + 1);  // 收缩左边界
        last[s[right]] = right;
        res = max(res, right - left + 1);
    }
    return res;
}
```

---

## 栈的应用

```cpp
// 有效括号
bool isValid(string s) {
    stack<char> st;
    for (char c : s) {
        if (c == '(' || c == '[' || c == '{') st.push(c);
        else {
            if (st.empty()) return false;
            if (c == ')' && st.top() != '(') return false;
            if (c == ']' && st.top() != '[') return false;
            if (c == '}' && st.top() != '{') return false;
            st.pop();
        }
    }
    return st.empty();
}

// 单调栈：下一个更大元素
vector<int> nextGreater(vector<int>& nums) {
    int n = nums.size();
    vector<int> res(n, -1);
    stack<int> st;  // 存下标，维护单调递减
    for (int i = 0; i < n; i++) {
        while (!st.empty() && nums[i] > nums[st.top()]) {
            res[st.top()] = nums[i];
            st.pop();
        }
        st.push(i);
    }
    return res;
}
```

---

## 面试常问

**Q：快排最坏情况是什么，怎么避免？**

每次选到最大或最小元素作为 pivot，退化为 O(N²)，常见于有序输入。避免方法：随机选 pivot，或三数取中法。`std::sort` 用 IntroSort（快排 + 堆排 + 插入排序的混合），保证最坏 O(N log N)。

**Q：什么时候用 BFS，什么时候用 DFS？**

- BFS：求最短路径（无权图）、按层处理、找最近的节点
- DFS：遍历所有路径、连通性判断、拓扑排序、回溯

**Q：动态规划和递归+记忆化的区别？**

本质相同，都是避免重复子问题的计算。记忆化是自顶向下（递归），DP 是自底向上（迭代）。DP 没有递归栈开销，空间通常可以进一步压缩（滚动数组）。
