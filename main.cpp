#include "main.h"
#include "disjoint-set.h"
#include "mouse.h"


//画像読み込み関数
/*
void loadImg(){
    input  = imread(img, CV_LOAD_IMAGE_COLOR);
    width = input.cols;
    height = input.rows;
    cout << "input: " << width << "x" << height << endl;
}
*/

//エッジリストへの追加
void AddEdges(int j, int i, int k, float w){
    edges[num_edge].w = w;
    edges[num_edge].a = j * width + i;
    edges[num_edge].b = (j + neigh[k][1]) * width + i + neigh[k][0];
    num_edge++;
}

inline int isIn(int x, int y, int w, int h){
    return (x>=0 && x<w && y>=0 && y<h);
}

//グラフ作成(重み付け)関数
void makeGraph(){
    edges=(edge*)malloc(sizeof(edge) * height * width * 4); //エッジのメモリ割当
    for(int j=0; j<height - 1; j++){
        for(int i=0; i<width - 1; i++){
            Vec3b pixel = frame.at<Vec3b>(j,i);
            uchar red = pixel[2];
            uchar green = pixel[1];
            uchar blue = pixel[0];
            
            //隣接ノードとの色空間でのユークリッド距離を計算(L2ノルム)
            for(int k=0; k<4; k++){
                if(isIn(i+neigh[k][0], j+neigh[k][1], width, height)){
                    Vec3b n_pixel = frame.at<Vec3b>(j + neigh[k][1], i + neigh[k][0]);
                    uchar n_red = n_pixel[2];
                    uchar n_green = n_pixel[1];
                    uchar n_blue = n_pixel[0];
                    
                    uchar dr = red - n_red;
                    uchar dg = green - n_green;
                    uchar db = blue - n_blue;
                    
                    //Edge.at<Vec4f>(j,i)[k] = sqrt(dr*dr + dg*dg + db*db);
                    //cout << "i,j,k = " << i << j << k << endl;
                    AddEdges(j,i,k,sqrt(dr*dr + dg*dg + db*db));
                }
            }
        }
    }
}

int compEdge(const void *a, const void *b)
{
    return (int)(((edge*)b)->w - ((edge*)a)->w);
}

//色をランダムに選択
rgb random_rgb(){ 
  rgb c;
  double r;
  
  c.r = (uchar)random();
  c.g = (uchar)random();
  c.b = (uchar)random();
  return c;
}

//グラフ分割関数
universe *segmentGraph(int num_vertices, float c){
    //qsort(EL,num_edge,sizeof(EdgeList),compEL); //エッジの重みで降順にソート
    qsort(edges,num_edge,sizeof(edge),compEdge);
    universe *u = new universe(num_vertices);
    float *threshold = new float[num_vertices];
    for(int i=0; i<num_vertices; i++) threshold[i] = THRESHOLD(1,c); //閾値関数を初期化
    for(int i=0; i<num_edge; i++){
        edge *pedge = &edges[i];
    
        // components conected by this edge
        int a = u->find(pedge->a);
        int b = u->find(pedge->b);
        if (a != b) {
            if ((pedge->w <= threshold[a]) && (pedge->w <= threshold[b])) {
                u->join(a, b);
                a = u->find(a);
                threshold[a] = pedge->w + THRESHOLD(u->size(a), c);
            }
        }
    }
    delete [] threshold;
    return u;
}

//領域分割関数
Mat segmentImage(int num_vertices, universe *u, int min_size, int *num_ccs){
    Mat output(height, width, CV_8UC3);
    for(int i=0; i<num_edge; i++){
        int a = u->find(edges[i].a);
        int b = u->find(edges[i].b);
        if ((a != b) && ((u->size(a) < min_size) || (u->size(b) < min_size)))
            u->join(a, b);
    }
    //delete [] edges;
    *num_ccs = u->num_sets();

    //色ぬり
    rgb *colors = new rgb[num_vertices];
    for (int i = 0; i < num_vertices; i++) colors[i] = random_rgb();
    for(int j=0; j<height; j++){
        for(int i=0; i<width; i++){
            int comp = u->find(j * width + i);
            output.at<Vec3b>(j,i)[0] = colors[comp].b;
            output.at<Vec3b>(j,i)[1] = colors[comp].g;
            output.at<Vec3b>(j,i)[2] = colors[comp].r;
        }
    }

    //エッジを黒く
    for(int i=0; i<num_edge; i++){
        if(u->find(edges[i].a) != u->find(edges[i].b)){
/*            int ax = edges[i].a % width;
            int ay = edges[i].a / width;
*/
            int bx = edges[i].b % width;
            int by = edges[i].b / width;
            if(/*isIn(ax,ay,width,height) && */isIn(bx,by,width,height)){
/*                output.at<Vec3b>(ay,ax)[0] = 0;
                output.at<Vec3b>(ay,ax)[1] = 0;
                output.at<Vec3b>(ay,ax)[2] = 0;
*/
                output.at<Vec3b>(by,bx)[0] = 0;
                output.at<Vec3b>(by,bx)[1] = 0;
                output.at<Vec3b>(by,bx)[2] = 0;
            }
        }
    }

    delete [] edges;

    return output;
}

//引数 sigma k min 
int main(int argc, const char* argv[]){
    if (argc != 4) {
        fprintf(stderr, "usage: %s sigma k min \n", argv[0]);
        return 1;
    }

    float sigma = atof(argv[1]); //平滑化の度合い
    float k = atof(argv[2]); //しきい値関数のパラメータ
    int min_size = atoi(argv[3]); //最小分割数

    int num_ccs; //コンポーネントの数
    int prev = 0;
    int frame_num = 0;
    int bar_value;

    //画像読み込み
    //loadImg();

    //映像読み込み
    //webカメラからの映像を入力とする場合
    //VideoCapture cap(0);
    //動画ファイルを入力とする場合(ファイルパスはwebcam_color.hのmovie)
    VideoCapture cap(movie);
    if(!cap.isOpened()) return -1;

    width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
    height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
    cout << "入力映像サイズ:" << width << "x" << height << endl;
    frame.create(height,width,CV_8UC3);
    for (int i=0; i<BUF_N; i++) buf[i].create(height,width,CV_8UC3);
    //ボタン画像読み込み
    Mat button = imread("buttons/BUTTON.JPG", 1);
    Mat button_2 = imread("buttons//BUTTON_2.JPG", 1);
    Mat button_2a = imread("buttons/./BUTTON_2a.JPG", 1);
    //ボタンサイズを適切に
    double resize_y = (double)height / (double)480;
    resize(button, button, Size(), 1, resize_y);
    resize(button_2, button_2, Size(), 1, resize_y);
    resize(button_2a, button_2a, Size(), 1, resize_y);
    namedWindow("output", WINDOW_AUTOSIZE);
    setMouseCallback("output", onMouse, 0);
    createTrackbar("OUTPUT", "output", &bar_value, BUF_N-1);

    while(1){
        if(CAP == 0){
            cap >> frame;
            if(frame.empty()){
                cap.set(CV_CAP_PROP_POS_FRAMES,0.0);
                continue;
            }
        } else {
            prev = frame_num;
            frame_num = MAX(0, getTrackbarPos("OUTPUT", "output")-1);
        }
        num_edge = 0;

        if(frame_num != prev){
            //cvSmooth(frame, frame, CV_BILATERAL, 11, 11, 50, 100);
            //bilateralFilter(frame, frame, 10, 100, 10, BORDER_DEFAULT);
            //GaussianBlur(frame, frame, Size(7,5), 8, 6);
            medianBlur(frame, frame, 3);
            makeGraph();
            universe *u = segmentGraph(width*height, k);
            output = segmentImage(width*height, u, min_size, &num_ccs);
            output.copyTo(buf[frame_num]);
        } 
        else if(frame_num == prev) {
            if(frame_num != 0)output = buf[frame_num-1];
            else output = frame;
        }

        hconcat(output, button, output);//ボタン画像と入力映像を横に連結
        if(CAP == 0) hconcat(output,button_2,output);//ボタン画像2と入力映像を横に連結
        else         hconcat(output,button_2a,output);//ボタン画像2と入力映像を横に連結

        imshow("output", output);
//      cout << frame_num << endl;
        if(CAP == 0){
            prev = frame_num;
            frame_num++;
            if(frame_num == BUF_N) frame_num = 0;
            setTrackbarPos("OUTPUT", "output", frame_num);
        }
        //Escを押すと終了
        if(waitKey(1) == 0x1b) return 0;
    }
}
