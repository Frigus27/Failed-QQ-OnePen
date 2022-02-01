// 一笔画 暴力破解
// QQ一笔画的神奇之处
// 在于图扩展前是欧拉迹，扩展后是哈密顿链
// 输入：
// 第一行：正方形边数
// 若干行：四个数字x,y,z,t，表示一条视觉上存在的边(x,y)--(z,t)，假定只连最近的边
// 输出：
// 一行，表示一条可能的路径

#include <iostream>
#include <sstream>
#include <tuple>
#include <cstdlib>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
using namespace std;


/*--------------------------------------------------
全局变量
--------------------------------------------------*/
using Point = int;
constexpr double FloatError = 0.00001;              // 当前精度下够用
constexpr int N = 5;                                // 最大边数
constexpr Point MaxPointIndex = N * N - 1;          // 最大可能点编号
constexpr int BufferSize = 128;

int n;                                              // 边数
int x, y, z, t;                                     // 用于表示一条边的临时变量
string buf;                                         // 输入buffer
istringstream iss;                                  // 转换用
vector<Point> oddDegPointPoints;                    // 用于检测扩展前图是否为欧拉迹，及确定可能起点
bool inputMap[N*N][N*N];                            // 扩展前的图，用于输入
bool extendedMap[N*N][N*N];                         // 扩展后的图，用于寻找哈密顿链
int inputDeg[N*N];                                  // 扩展前的图的各点度，用于判断欧拉迹存在
vector<Point> relevantPoints;                       // 有关联的所有点
vector<Point> result;                               // 搜索结果
bool succeeded = false;                             // 完成搜索

/*---------------------------------------------------
魔法宏，为了COOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOL!
---------------------------------------------------*/
#define IterateWithPoint(x) for (Point x = 0; x <= MaxPointIndex; x ++)
#define IterateWithPointStartAfter(x, y) for (Point x = y + 1; x <= MaxPointIndex; x ++)
#define IterateWithPointEndBefore(x, y) for (Point x = 0; x <= y - 1; x ++)
#define IterateWithPointWithSize(x, start, end) for (Point x = start; x <= end; x ++)


/*----------------------------------------------------
实用函数
----------------------------------------------------*/
;
inline bool isInteger(double number) { return (number - int(number) < FloatError); }
inline Point PosToPoint(int x, int y) { return y * n + x; }
inline decltype(auto) PointToPos(Point pointPoint) { return make_tuple(pointPoint % n, int(pointPoint / n)); }
inline decltype(auto) ShiftInput(int inputX, int inputY, int inputZ, int inputT) { return make_tuple(inputX - 1, inputY - 1, inputZ - 1, inputT - 1); }
inline decltype(auto) ShiftOutput(int outputX, int outputY) { return make_tuple(outputX + 1, outputY + 1); }
template <typename T> bool hasElement(vector<T> vec, T element) { return find(vec.begin(), vec.end(), element) != vec.end(); }

void ShowMap(bool map[N*N][N*N]) {
    IterateWithPoint(x) {
        IterateWithPoint(y) {
            if (x == y) {
                cout << "\\" << " ";
            }
            else {
                cout << int(map[x][y]) << "  ";
            }
        }
        cout << endl;
    }
}

void ShowMapGraph() {
    for (int i = 0; i < n; i ++) {
        for (int j = 0; j < n; j ++) {
            cout << PosToPoint(i, j) << " ";
        }
        cout << endl;
    }
}

bool isColinear(Point p1, Point p2, Point p3) {
    auto [x1, y1] = PointToPos(p1);
    auto [x2, y2] = PointToPos(p2);
    auto [x3, y3] = PointToPos(p3);
    if (x1 == x2 && x2 == x3) return true;
    if (y1 == y2 && y2 == y3) return true;
    if (double(x2 - x1) / double(x3 - x1) == double(y2 - y1) / double(y3 - y1)) return true;
    return false;
}

// 获取一条边覆盖的点
vector<Point> CoveredPoints(Point point1, Point point2) {
    vector<Point> coveredPoints = vector<Point>();
    auto [x1, y1] = PointToPos(point1);
    auto [x2, y2] = PointToPos(point2);
    if (x1 > x2) {
        swap(x1, x2);
        swap(y1, y2);
    }
    if (x1 == x2) {
        if (y1 > y2) {
            swap(x1, x2);
            swap(y1, y2);
        }
        for (int i = y1 + 1; i < y2; i ++) {
            coveredPoints.push_back(PosToPoint(x1, i));
        }
    }
    else {
        for (int xx = x1 + 1; xx < x2; xx ++) {
            double yx = double(xx - x1) / double(x2 - x1) * (y2 - y1) + y1;
            if (isInteger(yx)) {
                coveredPoints.push_back(PosToPoint(xx, int(yx)));
            }
        }
    }
    
    return coveredPoints;
}

/*----------------------------------------------
步骤
----------------------------------------------*/

void Init() {
    memset(inputMap, 0, sizeof(inputMap));
    memset(extendedMap, 0, sizeof(extendedMap));
    memset(inputDeg, 0, sizeof(inputDeg));
    oddDegPointPoints = vector<Point>();
    relevantPoints = vector<Point>();
}

void GetInput() {
    cout << "正方形边长：";
    cin >> n;
    cin.get();      // 吞掉回车
    while (true) {
        cout << "输入下一条边（空行结束）：";
        getline(cin, buf);
        if (buf.empty()) {
            break;
        }
        iss.str(buf);
        iss >> y >> x >> t >> z;
        auto [xs, ys, zs, ts] = ShiftInput(x, y, z, t);

        Point p1 = PosToPoint(xs, ys);
        Point p2 = PosToPoint(zs, ts);
        //建图
        inputMap[p1][p2] = true;
        inputMap[p2][p1] = true;
        inputDeg[p1] ++;
        inputDeg[p2] ++;
        if (!hasElement(relevantPoints, p1)) {
            relevantPoints.push_back(p1);
        }
        if (!hasElement(relevantPoints, p2)) {
            relevantPoints.push_back(p2);
        }
        iss.clear();
    }
}

bool CheckFirstMap() {
    // 检验是不是QQ一笔画
    IterateWithPoint(i) {
        if (inputDeg[i] % 2 == 1) {
            oddDegPointPoints.push_back(i);
        }
    }
    // 错误
    int possiblePointCount = oddDegPointPoints.size();
    if (possiblePointCount == 0) {
        cout << "你这不是QQ红包一笔画啊，baka" << endl;
        return false;
    }
    /*
    else if (possiblePointCount % 2 == 1) {
        cout << "这是图？" << endl;
        return false;
    }
    */
    else if (possiblePointCount > 2) {
        cout << "多起点，不是一笔画" << endl;
        return false;
    }
    // 剩下的就只有正常一笔画的情形了
    return true;
}

void ExtendMap() {
    memcpy(extendedMap, inputMap, sizeof(inputMap));
    bool hasChanged;
    // 冒 泡 延 展 算 法
    while (true) {
        hasChanged = false;
        IterateWithPoint(point1) {
            IterateWithPoint(point2) {
                if (point1 == point2) {
                    continue;
                }
                if (extendedMap[point1][point2] == true) {
                    IterateWithPoint(point3) {
                        if (point1 == point3 || point2 == point3) {
                            continue;
                        }
                        if (extendedMap[point1][point3] == true) {
                            if (extendedMap[point2][point3] == true) {
                                continue;
                            }
                            if (isColinear(point1, point2, point3)) {
                                extendedMap[point2][point3] = true;
                                extendedMap[point3][point2] = true;
                                hasChanged = true;
                            }
                        }
                    }
                }
            }
        }
        if (!hasChanged) {
            break;
        }
    }
}

bool isDfsSucceeded(vector<Point> currentResult, bool currentVisualMapHasEdge[N*N][N*N]) {
    // 1. 检测是否已走完哈密顿路
    vector<Point> tempResult = currentResult;
    vector<Point> workingRelevantPoints = relevantPoints;
    sort(workingRelevantPoints.begin(), workingRelevantPoints.end());
    sort(tempResult.begin(), tempResult.end());
    if (tempResult != workingRelevantPoints) {
        return false;
    }

    // 2. 检测是否已走完所有边
    bool flag = true;
    for (int i = 0; i < N*N; i ++) {
        for (int j = 0; j < N*N; j ++) {
            if (currentVisualMapHasEdge[i][j] == true) {
                //cout << "还有边" << i << "--" << j;
                flag = false;
            }
        }
    }
    return flag;
}

void Dfs(vector<Point> nextMoveRange, vector<Point> currentResult, bool currentVisualMap[N*N][N*N], int floor) {
    // 检测成功
    if (isDfsSucceeded(currentResult, currentVisualMap)) {
        result = currentResult;
        succeeded = true;
        return;
    }
    // 检测边界
    if (nextMoveRange.size() == 0) {
        return;
    }
    // 检测跨越
    if (currentResult.size() >= 2) {
        vector<Point> coveredPoints = CoveredPoints(currentResult[currentResult.size() - 1], currentResult[currentResult.size() - 2]);
        for (auto eachCoverPoint : coveredPoints) {
            if (!hasElement(currentResult, eachCoverPoint)) {
                return;
            }
        }
    }
    // 走nextMove
    bool newVisualMap[N*N][N*N];
    for (auto nextMove : nextMoveRange) {
        /* 调试信息
        cout << endl;
        for (int i = 0; i < floor; i ++) {
            cout << "\t";
        }
        cout << nextMove;*/

        // 消除掉对应的边
        for (int i = 0; i < N*N; i ++) {
            for (int j = 0; j < N*N; j ++) {
                newVisualMap[i][j] = currentVisualMap[i][j];
            }
        }
        if (currentResult.size() >= 1) {
            vector<Point> newCoveredPoints = CoveredPoints(currentResult[currentResult.size() - 1], nextMove);
            newCoveredPoints.push_back(currentResult[currentResult.size() - 1]);
            newCoveredPoints.push_back(nextMove);
            for (auto p1 : newCoveredPoints) {
                for (auto p2: newCoveredPoints) {
                    if (p1 == p2) continue;
                    //cout << "消" << p1 << "-" << p2;
                    newVisualMap[p1][p2] = false;
                    newVisualMap[p2][p1] = false;
                }
            }
        }
        //cout << "消边完毕";
        
        // 初步构造nextMove下的nextMoveRange
        vector<Point> newNextMoveRanges = vector<Point>();
        vector<Point> newCurrentResult = currentResult;
        newCurrentResult.push_back(nextMove);
        for (auto eachPoint : relevantPoints) {
            if (extendedMap[eachPoint][nextMove] == true && !hasElement(newCurrentResult, eachPoint)) {
                newNextMoveRanges.push_back(eachPoint);
            }
        }
        //cout << "构造完毕";
        // 继续dfs
        Dfs(newNextMoveRanges, newCurrentResult, newVisualMap, floor + 1);
        if (succeeded) {
            return;
        }
    }
}

void Dfs() {
    Dfs(oddDegPointPoints, vector<Point>(), inputMap, 0);
}

void Output() {
    cout << endl << "结果:" << endl;
    for (auto eachResult : result) {
        auto [y, x] = PointToPos(eachResult);
        auto [xs, ys] = ShiftOutput(x, y);
        cout << "(" << xs << "," << ys << ")" << endl;
    }
}

int main() {
    Init();
    GetInput();
    if (!CheckFirstMap()) {
        return 2;
    }
    ExtendMap();
    Dfs();
    Output();
    return 0;
}