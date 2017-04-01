void onButton(int x, int y){
    int button_id = 0;
    //ボタンの左列をクリック
    if(x < width + 100){
        int button_height = floor(height/8);
        button_id = y / button_height + 1;
    }
    
    //ボタンの右列をクリック
    else {
        int button_height = floor(height/8);
        button_id = y / button_height + 9;
    }
    
    cout << "ボタン" << button_id << "が押されました。" << endl;
    switch(button_id){
        case 1: //映像のキャプチャ切り替え
            if(CAP == 0) CAP = 1;
            else CAP = 0;
            break;
        case 2: //領域分割モード切り替え
            if(SEG == 0) SEG = 1;
            else SEG = 0;
            break;
        case 3: //書き出しモードON(OFFはなし)
            if(WRT == 0 && CAP == 1){
                WRT = 1;
                CAP = 0;
                SEG = 1;
                frame_num = 0;
            }
            break;
        default:
            break;
    }
}

void onMouse(int event, int x, int y, int flags, void*){
    if(event == CV_EVENT_LBUTTONDOWN){
        if(x >= width){
            onButton(x,y);
            return;
        }
    }
}