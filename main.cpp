#include "main.h"
#include "disjoint-set.h"
#include "mouse.h"

//エッジリストへの追加
void AddEdges(int j, int i, int k, float w){
    edges[num_edge].w = w;
    edges[num_edge].a = j * width + i;
    edges[num_edge].b = (j + neigh[k][1]) * width + i + neigh[k][0];
    num_edge++;
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
                    
                    AddEdges(j,i,k,sqrt(dr*dr + dg*dg + db*db));
                }
            }
        }
    }
}

int compEdge(const void *a, const void *b){
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
    qsort(edges,num_edge,sizeof(edge),compEdge);
    universe *u = new universe(num_vertices);
    float *threshold = new float[num_vertices];
    for(int i=0; i<num_vertices; i++) threshold[i] = THRESHOLD(1,c); //閾値関数を初期化
    for(int i=0; i<num_edge; i++){
        edge *pedge = &edges[i];

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
            int ax = edges[i].a % width;
            int ay = edges[i].a / width;
            int bx = edges[i].b % width;
            int by = edges[i].b / width;
            if(isIn(ax,ay,width,height) && isIn(bx,by,width,height)){
                output.at<Vec3b>(ay,ax)[0] = 0;
                output.at<Vec3b>(ay,ax)[1] = 0;
                output.at<Vec3b>(ay,ax)[2] = 0;
                /*
                output.at<Vec3b>(by,bx)[0] = 0;
                output.at<Vec3b>(by,bx)[1] = 0;
                output.at<Vec3b>(by,bx)[2] = 0;
                */
            }
        }
    }
    delete [] edges;

    return output;
}

//引数 sigma k min 
int main(int argc, const char* argv[]){
    //float sigma = atof(argv[1]); //平滑化の度合い(2017/03/30現在未使用)
    //float k = atof(argv[2]); //しきい値関数のパラメータ
    //int min_size = atoi(argv[3]); //最小分割数

    float k = 5.0, pre_k;
    int min_size = 250, pre_min_size;
    int num_ccs;

    int bar_value;
    int k_value;
    int min_size_value;

    //映像読み込み
    //webカメラからの映像を入力とする場合
    //VideoCapture cap(0);
    //動画ファイルを入力とする場合(ファイルパスはwebcam_color.hのmovie)
    VideoCapture cap(movie);
    if(!cap.isOpened()) return -1;

    width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
    height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
    float fps = cap.get(CV_CAP_PROP_FPS);

    //書き出し用
    VideoWriter writer("../../movie/output/OUTPUT.avi", VideoWriter::fourcc('D', 'I', 'V', '3'), fps, Size(height, width));
    if (!writer.isOpened()) return -1;

    cout << "入力映像サイズ:" << width << "x" << height << endl;
    cout << "フレームレート:" << fps << endl;
    frame.create(height,width,CV_8UC3);
    for (int i=0; i<BUF_N; i++) {
        f_buf[i].create(height,width,CV_8UC3);
        o_buf[i].create(height,width,CV_8UC3);
    }
    //ボタン画像読み込み
    Mat button_a = imread("buttons/button_a.png", 1);
    Mat button_b = imread("buttons/button_b.png", 1);
    Mat button_c = imread("buttons/button_c.png", 1);
    Mat button_d = imread("buttons/button_d.png", 1);
    //ボタンサイズを適切に
    double resize_y = (double)height / (double)480;
    resize(button_a, button_a, Size(), 1, resize_y);
    resize(button_b, button_b, Size(), 1, resize_y);
    resize(button_c, button_c, Size(), 1, resize_y);
    resize(button_d, button_d, Size(), 1, resize_y);
    namedWindow("output", WINDOW_AUTOSIZE);
    setMouseCallback("output", onMouse, 0);
    createTrackbar("frame", "output", &bar_value, BUF_N-1);
    createTrackbar("k","output", &k_value, K_MAX-1);
    createTrackbar("min_size","output", &min_size_value, MIN_SIZE_MAX-1);

    setTrackbarPos("k", "output", k*100);
    setTrackbarPos("min_size", "output", min_size);

    while(1){
        if(CAP){ //キャプチャされているとき
            frame_num = MAX(0, getTrackbarPos("frame", "output")-1);
            k = getTrackbarPos("k", "output") / (float)100.0;
            min_size = getTrackbarPos("min_size", "output");
            f_buf[frame_num].copyTo(frame);
        } else { //キャプチャされていないとき
            cap >> frame;
            if(frame.empty()){
                cap.set(CV_CAP_PROP_POS_FRAMES,0.0);
                continue;
            }
            frame.copyTo(f_buf[frame_num]);
        }

        num_edge = 0;

        if(SEG){ //分割モードで
            //キャプチャされてないまたはパラメータが変更された時
            if(!CAP || (fabsf(k - pre_k)) > EPSILON || (min_size != pre_min_size)){
                medianBlur(frame, frame, 3);
                makeGraph();
                universe *u = segmentGraph(width*height, k);
                output = segmentImage(width*height, u, min_size, &num_ccs);
                output.copyTo(o_buf[frame_num]);
            }
            //キャプチャされているとき
            else if(CAP){
                o_buf[frame_num].copyTo(output);
            }
        } else { //分割モードでないとき
            output = f_buf[frame_num];
        }

        //ボタン画像を連結
        if(CAP){ //キャプチャされていて
            if(SEG) hconcat(output,button_a,output); //セグメントモードのとき
            else    hconcat(output,button_b,output); //セグメントモードでないとき
        } else { //キャプチャされておらず
            if(SEG) hconcat(output,button_c,output); //セグメントモードのとき
            else    hconcat(output,button_d,output); //セグメントモードでないとき
        }

        //出力
        imshow("output", output);

        //書き出し
        if(WRT){
            writer << output;
            if(frame_num == 50) return 0;
        }

        if(!CAP){
            frame_num++;
            if(frame_num == BUF_N) frame_num = 0;
            setTrackbarPos("frame", "output", frame_num);
        }

        //前のパラメータを保存
        pre_k = k;
        pre_min_size = min_size;

        //Escを押すと終了
        if(waitKey(1) == 0x1b){
            destroyWindow("output");
            return 0;
        }
    }
}





