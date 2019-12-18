/*
 * @Author: Sauron Wu
 * @GitHub: wutianze
 * @Email: 1369130123qq@gmail.com
 * @Date: 2019-09-20 14:23:08
 * @LastEditors  : Sauron Wu
 * @LastEditTime : 2019-12-18 12:33:47
 * @Description: 
 */
#include <cmath>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctime>
#include <fstream>
#include <sstream>
#include <thread>
#include "safe_queue.h"
#include <mutex>
#include "control.h"
using namespace cv;
using namespace std;
using namespace std::chrono;

//int threadnum;
mutex mtxQueueStore;
safe_queue<pair<string,Mat>> queueStore;
#define NOTSTART 0
#define STARTRECORD 1
#define EXIT 2

#define STORESIZE_WIDTH 160
#define STORESIZE_HEIGHT 120

int imgMax = 50000;
int startRecord = NOTSTART;
//mutex mutexshow;
string path = "./images/";

void storeImage()
{
    ofstream outFile;
    outFile.open(path + "train.csv", ios::out | ios::app);
    int count = 0;
    while(count <= imgMax){
	sleep(0.05);
	mtxQueueStore.lock();
        if(startRecord == EXIT){
            mtxQueueStore.unlock();
            break;
        }else if(startRecord == NOTSTART){
            mtxQueueStore.unlock();
            continue;
        }
        mtxQueueStore.unlock();
        pair<string,Mat>tmpQ;
        queueStore.wait_and_pop(tmpQ);
        clock_t now = clock();
        string fileName = to_string(now) + ".jpg";
        imwrite(path + fileName, tmpQ.second);
        outFile << fileName << ','<< tmpQ.first <<endl;
        count++;
	if(count%1000 ==0)cout<<"now count:"<<count<<endl;
            }
    outFile.close();
    cout<<"get enough imgs:"<<count<<"\n";
}

int main(int argc, char **argv)
{
    if (argc != 4) {
          cout << "Usage of this exe: ./collect 50000(img number to collect) 0.23(run speed) 0/1(1 means see the image)"
             << endl;
        return -1;
      }
    imgMax = atoi(argv[1]);
    float runSpeed = atof(argv[2]);
    int seeImg = atoi(argv[3]);
    PYNQZ2 controller = PYNQZ2();

    VideoCapture cap(0);
    cap.set(CV_CAP_PROP_FRAME_WIDTH, 160);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, 120);
    Mat image;
    Mat resizeImage;
    if (access(path.c_str(), 0) == -1)
    {
        mkdir(path.c_str(), 0777);
    }
    if(seeImg == 0)namedWindow("just get key");
    thread storeThread = thread(storeImage);
    while (startRecord != EXIT)
    {
        cap >> image;
        resize(image,resizeImage,Size(STORESIZE_WIDTH,STORESIZE_HEIGHT));
	if(seeImg==1)imshow("car see", resizeImage);
        char c = (char)waitKey(1);
        //cout << c << endl;
        //cout<<controller.nowS->steerRate<<endl;
	switch (c)
        {
        case 'w':
            //controller.steerSet(0);
            controller.throttleSet(-runSpeed);
            controller.setLeds(1);
            break;
        case 'a':
            controller.steerChange(-0.8);
            //controller.throttleSet(runSpeed);
            controller.setLeds(2);
            break;
        case 's':
            //controller.steerSet(0.2);
            controller.throttleSet(0);
            controller.setLeds(4);
            break;
        case 'd':
            controller.steerChange(0.8); 
            //controller.throttleSet(runSpeed);
            controller.setLeds(8);
            break;
	case 't':
            mtxQueueStore.lock();
            if(startRecord == NOTSTART)startRecord = STARTRECORD;
            else{
                startRecord = NOTSTART;
            }
            mtxQueueStore.unlock();
	        cout<<"Start or Stop Record\n";
	        break;
        case 27://Esc
            mtxQueueStore.lock();
            startRecord = EXIT;
            mtxQueueStore.unlock();
            break;
        default:
            {
                float nowF = controller.nowS->steerRate;
            if(nowF >0){
		    if(nowF-0.2>=0){
                controller.steerChange(-0.2);}
		    else{
		    controller.steerSet(0.0);
		    }
            }else if(nowF<0){
		    if(nowF+0.2<=0){
            controller.steerChange(0.2);}
		    else{
		    controller.steerSet(0.0);
		    }
            }
            }
            }
        string oneRecord = controller.to_record();
        queueStore.push(make_pair(oneRecord,resizeImage));
    }
    storeThread.join();
    destroyAllWindows();
    cap.release();
    return 0;
}
