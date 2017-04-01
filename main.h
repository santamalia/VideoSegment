#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <fstream>

using namespace std;
using namespace cv;

#define isIn(x,y,w,h) (x>=0 && x<w && y>=0 && y<h)
#define THRESHOLD(size, c) (c/size) //しきい値関数
#define BUF_N 100 //画像バッファの数
#define K_MAX 2000 //しきい値関数τ()のパラメータの上限
#define MIN_SIZE_MAX 1000 //最小分割数パラメータの上限
#define EPSILON 1e-10 //浮動小数点比較のしきい値

//画像バッファ
Mat f_buf[BUF_N]; //入力動画のバッファ
Mat o_buf[BUF_N]; //出力動画のバッファ

//入力ビデオのパス
const char *movie = "../../movie/input/MOVIE04_MIN.mp4";

//フラグ
int CAP = 0; //キャプチャ
int SEG = 1; //セグメント
int WRT = 0; //書き出し

Mat frame;  //入力フレーム
Mat output; //出力用
int width, height; //縦横px数
int frame_num = 0; //フレーム番号

//隣接ノードの添字配列
int neigh[4][2] = { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 } };
/*
エッジ
  [0]:ー and (x+1, y)
  [1]:\ and (x+1, y+1)
  [2]:| and (x, y+1)
  [3]:/ and (x-1, y+1)
*/

typedef struct {
    float w;
    int a, b;
} edge;

edge *edges; //エッジリスト
int num_edge = 0; //エッジリストに含まれるエッジの数

typedef struct { uchar r, g, b; } rgb;