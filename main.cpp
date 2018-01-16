#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <vector>
#include <cmath>
#include <opencv2/core/utility.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/calib3d/calib3d.hpp"
#include <opencv2/stitching.hpp>
#include "OmnirangeCamera.hpp"


int HorizonalCrossCount = 9;
int VerticalCrossCount = 6;

//dvide image
void ImgDiv(cv::Mat src, cv::Mat right, cv::Mat left){
  int width = src.cols/2;
  int height = src.rows;
  for(int i = 0; i < height; i++){
    cv::Vec3b *p_src   = src.ptr<cv::Vec3b>(i);
    cv::Vec3b *p_right = right.ptr<cv::Vec3b>(i);
    cv::Vec3b *p_left  = left.ptr<cv::Vec3b>(i);
    for(int j = 0; j < width; j++){
      p_left[j] = p_src[j];
      p_right[j] = p_src[j + width];
    }
  }
}

void ImgJoin(cv::Mat right, cv::Mat left, cv::Mat src){
  int width = src.cols/2;
  int height = src.rows;
  for(int i = 0; i < height; i++){
    cv::Vec3b *p_src   = src.ptr<cv::Vec3b>(i);
    cv::Vec3b *p_right = right.ptr<cv::Vec3b>(i);
    cv::Vec3b *p_left  = left.ptr<cv::Vec3b>(i);
    for(int j = 0; j < width; j++){
      p_src[j] = p_left[j];
      p_src[j + width] = p_right[j] ;
    }
  }
}

void ImgClip(cv::Mat src, cv::Mat dst,cv::Rect range){
  int width = range.width;
  int height = range.height;
  int offset_x = range.x;
  int offset_y = range.y;

  for(int i = 0; i < height; i++){
    cv::Vec3b *p_src   = src.ptr<cv::Vec3b>(i+offset_y);
    cv::Vec3b *p_dst   = dst.ptr<cv::Vec3b>(i);
    for(int j = 0; j < width; j++){
      p_dst[j] = p_src[j+ offset_x];
    }
  }
}

void yMap(cv::Mat src,cv::Mat dst){
  int width = src.cols;
  int dist_width  = width ;
  int height = src.rows;
  for(int y = 0; y < height; y++){
    for(int x = 0; x < dist_width; x++){
      int py = y - (height / 2);
      int px = x - (dist_width / 2);

      double y1 = (asin( (double)px/(dist_width/2) )/(M_PI/2.0)) * ((double)width/2.0);
      double y2 = y1*sin( (M_PI/2) + ((double)py/((double)height/2.0) ) * M_PI/2 );

      unsigned short real_y = (int) ((width / 2) +  y2 ) ;
      if(real_y > src.rows ){
	std::cout << "error" << std::endl;
      }
      dst.ptr< unsigned short >(y)[x] = real_y ;
    }
  }
}
void xMap(cv::Mat src,cv::Mat dst){
  int width = src.cols;
  int dist_width  =   dst.cols ;
  int height = src.rows;
  for(int y = 0; y < height; y++){
    for(int x = 0; x < dist_width; x++){
      int py = y - (height / 2);
      int px = x - (dist_width / 2);

      double x1 = (height/2)*sin(((double)py/((double)height/2.0) ) * M_PI/2 );
      unsigned short real_x = (int) ((height / 2) +  x1 );
      if(real_x > src.cols ){
	std::cout << "error" << std::endl;
      }
      dst.ptr< unsigned short >(y)[x] = real_x ;
    }
  }
}

void xMap2(cv::Rect src,cv::Mat dst){
  int width = src.width;
  int dist_width  = dst.cols;
  int height = src.height;
  for(int y = 0; y < height; y++){
    for(int x = 0; x < dist_width; x++){
      double py = y - (height / 2);
      double px = x - (dist_width / 2);
      // 0 < theta  < 2
      double theta = (px/((double)dist_width/2)) *(M_PI/2);
      double phi =  (py/((double)height/2)) * (M_PI / 2);

      double d_x = sin(theta)*cos(phi)*(dist_width/2);
      unsigned short real_x = (int) ((dist_width / 2) +  d_x );
      if(real_x > dst.cols ){
	std::cout << "width error" << std::endl;
      }
      dst.ptr< unsigned short >(y)[x] = real_x ;
      if(phi > (M_PI/2) && theta > (M_PI/2) ){
	std::cout << "range error" << std::endl;
      }
    }
  }
}


void yMap2(cv::Rect src,cv::Mat dst){
  int width = src.width;
  int dist_width  = dst.cols;
  int height = src.height;
  for(int y = 0; y < height; y++){
    for(int x = 0; x < dist_width; x++){
      double py = y - (height / 2);
      double px = x - (dist_width / 2);

      double theta = (px/((double)dist_width/2)) *(M_PI/2);
      double phi =  (py/((double)height/2)) * (M_PI / 2);
      double d_y = sin(phi)*(height/2);
      
      unsigned short real_y = (int) ((height / 2) +  d_y ) ;
      if(phi > (M_PI/2) && theta > (M_PI/2) ){
	std::cout << "range error" << std::endl;
      }
      dst.ptr< unsigned short >(y)[x] = real_y ;
    }
  }
}

void xMap3(cv::Rect src,cv::Mat dst){
  int width = src.width;
  int dist_width  = dst.cols;
  int height = src.height;
  for(int y = 0; y < height; y++){
    for(int x = 0; x < dist_width; x++){
      double py = y - (height / 2);
      double px = x - (dist_width / 2);
      // 0 < theta  < 2
      double theta = (px/((double)dist_width/2)) *(M_PI/2);
      double phi =  (py/((double)height/2)) * (M_PI / 2);

      //point on orthogonal projection
      double d_x = sin(theta)*cos(phi)*(dist_width/2);
      double d_y = sin(phi)*(height/2);
      
      double r = sqrt(pow(d_x,2)+pow(d_y,2));
      double r_angle = asin(r/(height/2));
      //correct by focal length
      //double f = (height/height/2)/sin(r_angle/2)/2;
      double fr = tan(r_angle/2);

      double rate = fr/(r/(height/2));
      
      double c_x = d_x*rate;
      double c_y = d_y*rate;
      
      unsigned short real_x = (int) ((dist_width / 2) +  c_x );
      if(real_x > dst.cols ){
	std::cout << "width error" << std::endl;
      }
      dst.ptr< unsigned short >(y)[x] = real_x ;
      if(real_x < 0|| width < real_x  ){
	std::cout << "range error" << std::endl;
      }
    }
  }
}


void yMap3(cv::Rect src,cv::Mat dst){
  int width = src.width;
  int dist_width  = dst.cols;
  int height = src.height;
  for(int y = 0; y < height; y++){
    for(int x = 0; x < dist_width; x++){
      double py = y - (height / 2);
      double px = x - (dist_width / 2);
      
      //spherical coordinate
      double theta = (px/((double)dist_width/2)) *(M_PI/2);
      double phi =  (py/((double)height/2)) * (M_PI / 2);

      //point on orthogonal projection
      double d_x = sin(theta)*cos(phi)*(dist_width/2);
      double d_y = sin(phi)*(height/2);
      
      double r = sqrt(pow(d_x,2)+pow(d_y,2));
      double r_angle = asin(r/(height/2));
      //correct by focal length
      // double f = (height/height/2)/sin(r_angle)/2;
      double fr = tan(r_angle/2);

      double rate = fr/(r/(height/2));
      
      double c_x = d_x*rate;
      double c_y = d_y*rate;
      
      unsigned short real_y = (int) ((height / 2) +  c_y ) ;
      if(real_y < 0|| height < real_y  ){
	std::cout << "range error" << std::endl;
      }

      dst.ptr< unsigned short >(y)[x] = real_y ;
    }
  }
}




void omniremap(cv::Mat src, cv::Mat dst, cv::Mat xmap, cv::Mat ymap){
  int src_width = src.cols;
  int dst_width = dst.cols;
  if(xmap.cols != dst.cols || xmap.rows != dst.rows || ymap.cols != dst.cols || ymap.rows != dst.rows){
    std::cout << "error" << std::endl;
  }
  
  int height = xmap.rows;
  
  cv::Vec3b *p_src = src.ptr<cv::Vec3b>(0);      
  
  for(int y = 0 ; y < height; y++){
    cv::Vec3b *p_dst = dst.ptr<cv::Vec3b>(y);
    unsigned short *p_xmap  = xmap.ptr<unsigned short>(y);
    unsigned short *p_ymap  = ymap.ptr<unsigned short>(y);
    for(int x = 0 ; x < dst_width; x++){
      int index = p_ymap[x] * src_width + p_xmap[x];
      p_dst[x] = p_src[index];
    }
  }
}

int main(int argc , char* argv[]){
  if(argc != 2){
    std::cout << "too few argments" << std::endl;
    return -1;
  }

  int cam_id = std::atoi(argv[1]); 
  int clip = 0; 
  std::cout << "s to capture" << std::endl;
  std::cout << "ESC s to Exit Program" << std::endl;

  //set camera configration
  cv::VideoCapture insta360(cam_id);
  insta360.set(CV_CAP_PROP_FRAME_WIDTH, 1920);
  insta360.set( CV_CAP_PROP_FRAME_HEIGHT, 1080);  
  
  cv::Mat src;
  insta360 >> src;
  cv::Rect roi = cv::Rect(clip,clip,src.cols/2 - (2*clip) ,src.rows - (2*clip) );
  int change = 1;

  while(1){

    int key = cv::waitKey(1);
    if(key == 49){
      if(0 < clip ){
	clip--;
	std::cout << "clip range is " << clip <<std:: endl;
	roi = cv::Rect(clip,clip,src.cols/2 - (2*clip) ,src.rows - (2*clip) );
	change = 1;
      }  
    }else if (key == 50){
      if(0 < 1000 ){
	clip++;
	std::cout << "clip range is " << clip <<std:: endl;
	roi = cv::Rect(clip,clip,src.cols/2 - (2*clip) ,src.rows - (2*clip) );
	change = 1;
      }
    }else if(key == 27){
      break;
    }
    
    cv::Mat y_mat(roi.width, roi.height,CV_16UC1);
    cv::Mat x_mat(roi.width, roi.height,CV_16UC1);
    
    xMap3(roi,x_mat); 
    yMap3(roi,y_mat); 
    
    if(change == 1){
      std::cout << "change" << std::endl;
      xMap3(roi,x_mat); 
      yMap3(roi,y_mat); 
      change = 0;      
    }
    
    //fetch image
    insta360 >> src;
    cv::Mat right(src.cols/2, src.rows,right.type());
    cv::Mat left(src.cols/2, src.rows,right.type()); 
    ImgDiv(src,right,left);
    cv::Mat cliped_r(roi.width, roi.height,src.type());
    cv::Mat cliped_l(roi.width, roi.height,src.type());
    
    ImgClip(right,cliped_r,roi);
    ImgClip(left,cliped_l,roi);
    
    cv::Mat result_right(x_mat.rows, x_mat.cols, src.type());
    cv::Mat result_left(x_mat.rows, x_mat.cols, src.type());
       
    cv::imshow("debug2",cliped_r);
    
    omniremap(cliped_r, result_right,x_mat,y_mat);
    omniremap(cliped_l, result_left,x_mat,y_mat);
    
    cv::Mat Join(result_right.rows ,result_right.cols*2, result_right.type() );
    ImgJoin(result_right,result_left,Join);
    cv::imshow("result_r",Join);
    
  }
}