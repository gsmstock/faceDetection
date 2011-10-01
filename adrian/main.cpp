#include "CImg.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <iostream>
#include <vector>

using namespace cimg_library;
using namespace std;

#define MINH 0.0
#define MAXH 50.0
#define MINS 0.23
#define MAXS 0.68

class Region{
public:
    int x_min,x_max,y_min,y_max;
	int label;
	int x_cen,y_cen;
	bool isFace;

	Region(int label,int x_min,int x_max,int y_min,int y_max):label(label),x_min(x_min),x_max(x_max),y_min(y_min),y_max(y_max),x_cen(0),y_cen(0),isFace(false){};

	void check(int x, int y){
		if(x<x_min) x_min=x;
		if(x>x_max) x_max=x;
		if(y<y_min) y_min=y;
		if(y>y_max) y_max=y;
	}
	
	void get_center(){
		x_cen=(x_min+x_max)/2;
		y_cen=(y_min+y_max)/2;
	}

	void is_face(CImg <double> image){
		double width=x_max-x_min;
		double height=y_max-y_min;
		double stosunek;
		stosunek=height/width;
		double goldenratio=(1+sqrt(5.0))/2;
		double tolerance = 0.65;
		int ile;
		double skin_percentage;
		ile=0;
		for(int x=x_min;x<x_min+width+1;x++){
			for(int y=y_min;y<y_min+height+1;y++){
				if(image(x,y)!=0)
					ile++;
			}
		}
		double pom=width*height;
		skin_percentage=ile/pom;
		if(skin_percentage > 0.53 && (stosunek > goldenratio-tolerance && stosunek < goldenratio+tolerance) ){
			isFace=true;
		}
	}

 }; 

CImg <double> wykrywanieSkory(CImg <double> imageHSV) {
	cimg_forXY(imageHSV,x,y) {
		double valH = imageHSV(x,y,0,0);
		double valS = imageHSV(x,y,0,1);
		double valV = imageHSV(x,y,0,2);
		if (!(((MINH <= valH) && (valH <= MAXH)) && ((MINS < valS) && (valS <= MAXS)))) {
			imageHSV(x,y,0,0) = 0.0;
			imageHSV(x,y,0,1) = 0.0;
			imageHSV(x,y,0,2) = 0.0;
		}
	}
	return imageHSV;
}

CImg <double> wykrywanieRegionow(CImg <double> image) {
	CImg <double> skin_regions(image.width(),image.height());
	int x,y,x_curr,y_curr,ile;
	x_curr=y_curr=ile=0;
	bool czyKoniec=false;

	while(!czyKoniec){
		ile=0;
		for(x=x_curr ; x<x_curr+5 ; x++){
			for(y=y_curr ; y<y_curr+5 ; y++){
				if(image(x,y)!=0)
					ile++;
			}
		}
		if(ile>10){
			for(x=x_curr ; x<x_curr+5 ; x++){
				for(y=y_curr ; y<y_curr+5 ; y++){
					if(x < image.width() && y < image.height())
					skin_regions(x,y)=1;
				}
			}
			
		}
		x_curr=x;
		if(x_curr > image.width()){
			if(y_curr > image.height()){
				czyKoniec=true;
			}
			else{
				x_curr=0;
				y_curr+=5;
			}
		}
	}
	return skin_regions;
}

int ile_etykiet(CImg <double> skin_label){
	int liczba_etykiet_skory=0;
	cimg_forXY(skin_label,x,y){
		if(skin_label(x,y)>liczba_etykiet_skory)
			liczba_etykiet_skory = skin_label(x,y);
	}
	return liczba_etykiet_skory;
}

int main(){
CImg <double> image("1.jpg");
CImg <double> imageHSV;
CImg <double> image_final;
CImg <double> skin_regions;
CImg <double> skin_label;
const unsigned char red[] = { 255,0,0 }, green[] = { 0,255,0 }, blue[] = { 0,0,255 };
int liczba_etykiet_skory=0; 

//konwersja zdjêcia to HSV
imageHSV = image.get_RGBtoHSV();
//wykrywanie skóry z urzyciem HSV
imageHSV = wykrywanieSkory(imageHSV);
//utworzenie wiêkszych regionów
skin_regions=wykrywanieRegionow(imageHSV.get_HSVtoRGB());
//nadanie regionom etykiet
skin_label = skin_regions.get_label(true);
//zlicza etykiety skóry
liczba_etykiet_skory=ile_etykiet(skin_label);

//zlicza pixele w etykietach
int *ile_pixeli = new int[liczba_etykiet_skory+1]; 
for(int i=0; i<=liczba_etykiet_skory; i++){
		ile_pixeli[i] = 0;
}
int i=0;
cimg_forXY(skin_label,x,y) {
		i = skin_label(x,y);
		ile_pixeli[i]++;
}

//usuwa male fragmenty, ¿eby oczyœciæ obraz
for(int i=0; i<=liczba_etykiet_skory; i++){
		if(ile_pixeli[i]<250){
			ile_pixeli[i] = 0;
		}
}
cimg_forXY(skin_label,x,y){
	i = skin_label(x,y);
	if(ile_pixeli[i]==0){
		skin_label(x,y)=0;
	}
}

//tworzy vector klas regionów, ka¿demu regionowi nadaje pocz¹tkowe wartoœci
vector <Region> reg;
int j=1;
cimg_forXY(skin_label,x,y){
	j = skin_label(x,y);
	if(ile_pixeli[j]>1){
		Region temp(j,x,x+5,y,y+5);
		reg.push_back(temp);
		ile_pixeli[j]=1;
	}
}
//znajdowanie skrajnych wartoœci pixeli(najbardziej po lewo/prawo, najbardziej wysuniêty w dó³/górê)
i =0 ;
ile_pixeli[0]=0;
cimg_forXY(skin_label,x,y){
	j = skin_label(x,y);
	if(ile_pixeli[j]>0){
		for(int k=0;k<reg.size();k++){
			if(j==reg[k].label){
				reg[k].check(x,y);
			}
		}
	}
}
//znajdowanie œrodka obszaru na podstawie wartoœci skrajnych pixeli
for(int k=0;k<reg.size();k++){
	reg[k].get_center();
}

//dla ka¿dego regionu sprawdza czy to twarz, na podstawie z³ote proporcji +- tolerancja i %pixeli twarzy w obszarze
for(int k=1;k<reg.size();k++){
	reg[k].is_face(imageHSV.get_HSVtoRGB());
}
//dla znalezionej tworzy rysuje w œrodku czerwon¹ kropkê
image_final=image;
for(int k=0;k<reg.size();k++){
	if(reg[k].isFace)
	image_final.draw_circle(reg[k].x_cen,reg[k].y_cen,10,red);
	//image.draw_rectangle(reg[k].x_min,reg[k].y_min,reg[k].x_max,reg[k].y_max,green,0.25);
}
//wyœwietla zdjêcia

(image,imageHSV.get_HSVtoRGB(),skin_regions,skin_label,image_final).display();
system("pause");
return 0;
}
