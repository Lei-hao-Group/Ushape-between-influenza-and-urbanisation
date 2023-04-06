#include <QGuiApplication>
#include <QString>
#include <QImage>
#include "csvfile.h"
#include <QFile>
#include <QTextStream>
#include <iostream>
#include <QDebug>
#include <math.h>
#include <QDate>
#include <QDataStream>
#include <QRandomGenerator>
#include <random>

#define TARGET  1           //1是真实城市模拟，2是U型曲线模拟(CITYCODE要设置为北京）
#define ROUND   3             //一共重复多少轮
#define PEOPLENUM   1000000
#define STARTINFECT     20  //初始感染人数
#define DETA        2.22       //
#define CITYCODE        2      //城市序号，按拼音从1-13号，北京2，甘肃4，河北9，河南10，黑龙江11，吉林14，辽宁17，内蒙古18，宁夏19，青海20，山东21，山西22，陕西23，天津26，新疆28
#define COEF_B      0      //气候公式的系数之一
#define STARTYEAR   2013
#define STARTMONTH  11           //start自动是这个月的第一天
#define ENDYEAR     2014
#define ENDMONTH    6           //end自动到这个月最后一天
#define OFFICECONNECTPEO 4      //平均在办公室有效接触多少人
#define OFFICESIZE       15      //平均每个办公室的人数
#define COMPANYSIZE      100    //每个大公司的人数
#define OFFICEOTHERCONTACTPROB  0.02     //外办公室接触率
#define COMMUNITYSIZE   90  //平均每个community多少人
#define CLASSCONNECTPEO 9      //平均在学校接触多少人
#define CLASSSIZE       40      //平均每个班级的人数
#define MINSCHOOLSIZE      350    //平均在学校的最小人数，设置范围用，尽量节省空间的意思
#define MAXSCHOOLSIZE      1600    //平均在学校的最小人数，设置范围用，尽量节省空间的意思
#define SCHOOLOTHERCONTACTPROB  0.01     //外班接触率
#define WORKSTART       9   //工作开始时间
#define WORKEND         17  //工作结束时间
#define HOMELEAVE       8   //出家门时间
#define HOMEBACK        19 //回家时间
#define LATENTPERIOD    48  //潜伏期时间
#define INFECTPERIOD    96  //传染期时间
#define RECOVERPERIOD   11111111 //康复且不会再感染的时间
#define BETAHOME        0.001 //家中感染率
#define BETACOMMUNITY   0.00015957 //公共区域中感染率
#define BETAOFFICE      0.001 //办公室感染率
#define BETASCHOOL      0.002 //学校感染率
#define BETACOEF          0.93             //用于等比调整所有BETA(1.633)
//#define BETACOEF          1.505              //用于等比调整所有BETA(Target==1; 1.505)
#define URBANRATE      0.85   //City1城市化率
#define R0TEST         0       //用于测试R0,1测试RO;0不测试
#define RANDOMSEED  0                   //定义是否随机0固定，1完全随机                //定义是否随机0固定，1完全随机
#define STUDENTVULNERABILITY    2   //学生易感性
#define WORKERVULNERABILITY    1   //工作者易感性
#define OTHERVULNERABILITY    2   //老人易感性

class ValidNumData
{
public:
    ValidNumData(){valid=0;homeinfect=0;officeinfect=0;schoolinfect=0;communityinfect=0;}
    int valid;         //没有infect时候=0，刚刚infect=1，然后往上加，步长1小时。
    int homeinfect;
    int officeinfect;
    int schoolinfect;
    int communityinfect;
};
ValidNumData validnum[ROUND]; //记录每组数据是否有效

class PeoData
{
public:
    PeoData(){infecttime=0;age=0;start=0;}
    int infecttime;         //没有infect时候=0，刚刚infect=1，然后往上加，步长1小时。
    int home;               //家的住址号
    int company;             //公司号码
    int companyoffice;      //办公室号码
    int school;             //学校号码
    int schoolclass;        //班级编号
    int community;          //公共区域号码
    short int age;      //定义人是工作还是学校，age=1学校，age=2工作
    short int start;            //起始感染1，起始不感染0
    int officeconnect;       //每个人接触几个office人
    int classconnect;       //每个人接触class几个人
    unsigned char vulnerability;
};
PeoData peo[PEOPLENUM]; //人员数据

class PeoClassfriendData
{
public:
    PeoClassfriendData(){peocode=0;}
    int peocode;         //好友的号码
};
PeoClassfriendData peoclassfriend[PEOPLENUM][int(CLASSCONNECTPEO*1.5)]; //每个人在学校的固定好友

class PeoOfficefriendData
{
public:
    PeoOfficefriendData(){peocode=0;}
    int peocode;         //好友的号码
};
PeoOfficefriendData peoofficefriend[PEOPLENUM][int(OFFICECONNECTPEO*1.5)]; //每个人在公司的固定好友

class AirDensityData
{
public:
    AirDensityData(){}
    double densitydry;         //干空气密度
};
AirDensityData airdensity[66]; //人员数据

class ProvinceData
{
public:
    ProvinceData(){count=0;counttemp=0;}
    int RHmax;         //相对湿度最大值
    int RHmin;          //相对湿度最小值
    double RHavg;       //平均相对湿度
    int count;      //某个省一共有多少站点
    double TEMPavg;    //平均温度
    int counttemp;  //某个省一共多少个温度站点
    double adjustR0;    //某个时间点的调整R0
};
ProvinceData prov[32][20][13][32]; //省份数据，31个省，20年(2000-2019)，12个月,31天

class ProvincePercentData
{
public:
    ProvincePercentData(){}
    double school;         //学校比例
    double work;            //工作比例
    double housesize;   //家庭数
    int schoolsize;      //每个学校的学生数
};
ProvincePercentData provpercent[32]; //省份数据

class StationData
{
public:
    StationData(){prov=0;}
    int prov;         //监测站对应的省份code
};
StationData station[60000]; //监测站

class StatisticData
{
public:
    StatisticData(){studentcommunity=0;studentschool=0;studenthome=0;workercommunity=0;workeroffice=0;workerhome=0;unemployedcommunity=0;unemployedhome=0;Snum=0;Inum=0;Rnum=0;homeinfect=0;communityinfect=0;officeinfect=0;schoolinfect=0;studentinfect=0;workerinfect=0;unemployedinfect=0;}
    int Snum;           //健康的总人数
    int Inum;         //感染的总人数
    int Rnum;           //康复的总人数
    int homeinfect;     //在家感染的总人数
    int communityinfect;    //在公共区域感染的总人数
    int officeinfect;       //在办公室感染的总人数
    int schoolinfect;       //在学校感染的总人数
    int studentinfect;      //学生感染总人数
    int workerinfect;       //工人感染总人数
    int unemployedinfect;   //失业者感染总人数
    int studentcommunity; //学生在社区感染人数
    int studentschool;          //学生在学校感染人数
    int studenthome;        //学生在家感染人数
    int workercommunity;      //工作者在社区感染人数
    int workeroffice;       //工作者在办公室感染人数
    int workerhome;         //工作者在家感染人数
    int unemployedcommunity;    //失业者在社区感染人数
    int unemployedhome;             //失业者在家感染人数z
};
StatisticData stat; //两城市的人员数据，0为总的

class SummaryData
{
public:
    SummaryData(){Snum=0;Inum=0;Rnum=0;homeinfect=0;communityinfect=0;officeinfect=0;schoolinfect=0;totalInum=0;totalSnum=0;}
    int Snum;           //健康的总人数
    int Inum;         //感染的总人数
    double totalInum;   //总感染人数，round的
    double totalSnum;   //
    int Rnum;           //康复的总人数
    int homeinfect;     //在家感染的总人数
    int communityinfect;    //在公共区域感染的总人数
    int officeinfect;       //在办公室感染的总人数
    int schoolinfect;       //在学校感染的总人数
    int peakday;            //峰值的时间
};
SummaryData summary[7500][ROUND]; //两城市的人员数据，0为总的


class HomeData
{
public:
    HomeData(){Snum=0;Inum=0;Rnum=0;population=0;}
    int Snum;           //健康的总人数
    int Inum;         //感染的总人数
    int Rnum;           //康复的总人数
    int population;     //home总人数
};
HomeData home[int(PEOPLENUM/2)]; //人员数据

class OfficeData
{
public:
    OfficeData(){Snum=0;Inum=0;Rnum=0;}
    int Snum;           //健康的总人数
    int Inum;         //感染的总人数
    int Rnum;           //康复的总人数
};
OfficeData office[int(PEOPLENUM/OFFICESIZE)]; //人员数据


class CompanyOfficeStepData
{
public:
    CompanyOfficeStepData(){}
    int peocode;     //对应的peo号码
};
CompanyOfficeStepData companyofficestep[int(PEOPLENUM/COMPANYSIZE+1)][int(COMPANYSIZE/OFFICESIZE)+1][OFFICESIZE+2]; //人员数据

class CompanyOfficeData
{
public:
    CompanyOfficeData(){linshipeonum=0;}
    int linshipeonum;   //临时的累计人数
};
CompanyOfficeData companyoffice[int(PEOPLENUM/COMPANYSIZE+1)][int(COMPANYSIZE/OFFICESIZE)+1]; //人员数据

class SchoolData
{
public:
    SchoolData(){Snum=0;Inum=0;Rnum=0;}
    int Snum;           //健康的总人数
    int Inum;         //感染的总人数
    int Rnum;           //康复的总人数
};
SchoolData school[int(PEOPLENUM/MINSCHOOLSIZE)]; //人员数据

class SchoolClassStepData
{
public:
    SchoolClassStepData(){}
    int peocode;     //对应的peo号码
};
SchoolClassStepData schoolclassstep[int(PEOPLENUM/MINSCHOOLSIZE)][int(MAXSCHOOLSIZE/CLASSSIZE)+1][CLASSSIZE+5]; //人员数据

class SchoolClassData
{
public:
    SchoolClassData(){linshipeonum=0;}
    int linshipeonum;   //临时的累计人数
};
SchoolClassData schoolclass[int(PEOPLENUM/MINSCHOOLSIZE)][int(MAXSCHOOLSIZE/CLASSSIZE)+1]; //人员数据

class CommunityData
{
public:
    CommunityData(){Snum=0;Inum=0;Rnum=0;peonum=0;}
    int Snum;           //健康的总人数
    int Inum;         //感染的总人数
    int Rnum;           //康复的总人数
    int peonum;          //某个community能承载多少人
    int currentpeonum;
    int recordlist;          //记录每个community是否满的list
};
CommunityData community[PEOPLENUM]; //人员数据

double RANDOM01(int a) //生成0-1之间的随机数
{
    return a*(rand()/double(RAND_MAX+1));
}
int RANDOM0N(int a,int b)//生成a-b之间的随机数（包括a，不包括b）
{
    return (a+int((b-a)*(rand()/double(RAND_MAX+1))));
}

int homenum;
int communitynum=0;
int totalstep=0;
int biground=0;
int communitysize;
int companynum;
int officenum;
int schoolnum;
int numofficeconnect;

void ClimateDataLoad(){
#if TARGET==1
    QString FileName_PERT="D:/QTprogramming/LeiHao/delta/climatedata/percent.csv";   //导入工作和上学的比例
    CSVFile reader_PERT;
    reader_PERT.readFile(FileName_PERT);
    provpercent[CITYCODE].school=reader_PERT.getData(1,CITYCODE).toDouble();
    provpercent[CITYCODE].work=reader_PERT.getData(2,CITYCODE).toDouble();
    provpercent[CITYCODE].housesize=reader_PERT.getData(3,CITYCODE).toDouble();
    communitysize=COMMUNITYSIZE*reader_PERT.getData(4,CITYCODE).toDouble()/0.131115;
    provpercent[CITYCODE].schoolsize=reader_PERT.getData(5,CITYCODE).toDouble();
#endif
    QString FileName_SC="D:/QTprogramming/LeiHao/delta/climatedata/weatherstationcode.csv";   //stationcode导入
    CSVFile reader_SC;
    reader_SC.readFile(FileName_SC);
    for (int i=1;i<840;i++){
        station[reader_SC.getData(0,i).toInt()].prov=reader_SC.getData(1,i).toInt();
    }

    QString FileName_AD="D:/QTprogramming/LeiHao/delta/climatedata/airdensity.csv";       //airdensity导入
    CSVFile reader_AD;
    reader_AD.readFile(FileName_AD);
    for (int i=0;i<56;i++){             //温度需要减20。例如airdensity[0]其实是-20度的空气密度
        airdensity[i].densitydry=reader_AD.getData(1,i+1).toDouble();
    }
    QFile qf_2("D:/QTprogramming/LeiHao/delta/climatedata/adjustR0.csv");
    QTextStream qts_2(&qf_2);
    qf_2.open(QIODevice::WriteOnly|QIODevice::Text);

    for (int year=2000; year<2019;year++){
        for (int month=1;month<13;month++){
            int monthday[13];
            if(month==1 || month==3 || month==5 || month==7 || month==8 || month==10 || month==12){
                monthday[month]=31;
            }else if(month==4 || month==6 || month==9 || month==11){
                monthday[month]=30;
            }else{
                if(year==2000 || year==2004 || year==2008 || year==2012 || year==2016){
                    monthday[month]=29;
                }else{
                    monthday[month]=28;
                }
            }
            QString FileName_RH="D:/QTprogramming/LeiHao/delta/climatedata/SURF_CLI_CHN_MUL_DAY-RHU-13003-"+QString::number(year)+QString::number(month).rightJustified(2,'0')+".csv";
            QFile qf_RH(FileName_RH);
            if(qf_RH.exists()==true){
                CSVFile reader_RH;
                reader_RH.readFile(FileName_RH);
                for (int i=0;i<50000;i++){
                    if(reader_RH.getData(0,i)!=""){
                        prov[station[reader_RH.getData(0,i).toInt()].prov][reader_RH.getData(4,i).toInt()-2000][reader_RH.getData(5,i).toInt()][reader_RH.getData(6,i).toInt()].RHmax+=reader_RH.getData(7,i).toInt();
                        prov[station[reader_RH.getData(0,i).toInt()].prov][reader_RH.getData(4,i).toInt()-2000][reader_RH.getData(5,i).toInt()][reader_RH.getData(6,i).toInt()].RHmin+=reader_RH.getData(8,i).toInt();
                        prov[station[reader_RH.getData(0,i).toInt()].prov][reader_RH.getData(4,i).toInt()-2000][reader_RH.getData(5,i).toInt()][reader_RH.getData(6,i).toInt()].count++;
                    }
                }
            }
            for (int day=1;day<32;day++){
                for (int province=1;province<32;province++){
                    if(prov[province][year-2000][month][day].count>0){
                        prov[province][year-2000][month][day].RHavg=(prov[province][year-2000][month][day].RHmax+prov[province][year-2000][month][day].RHmin)/2.0/double(prov[province][year-2000][month][day].count);
                    }
                }
            }

            QString FileName_Temp="D:/QTprogramming/LeiHao/delta/climatedata/SURF_CLI_CHN_MUL_DAY-TEM-12001-"+QString::number(year)+QString::number(month).rightJustified(2,'0')+".csv";
            QFile qf_Temp(FileName_Temp);
            if(qf_Temp.exists()==true){
                CSVFile reader_Temp;
                reader_Temp.readFile(FileName_Temp);
                for (int i=0;i<50000;i++){
                    if(reader_Temp.getData(0,i)!=""){
                        prov[station[reader_Temp.getData(0,i).toInt()].prov][reader_Temp.getData(4,i).toInt()-2000][reader_Temp.getData(5,i).toInt()][reader_Temp.getData(6,i).toInt()].TEMPavg+=reader_Temp.getData(7,i).toInt();
                        prov[station[reader_Temp.getData(0,i).toInt()].prov][reader_Temp.getData(4,i).toInt()-2000][reader_Temp.getData(5,i).toInt()][reader_Temp.getData(6,i).toInt()].counttemp++;
                    }
                }
            }
            for (int day=1;day<32;day++){
                for (int province=1;province<32;province++){
                    if(prov[province][year-2000][month][day].counttemp>0){
                        prov[province][year-2000][month][day].TEMPavg=(prov[province][year-2000][month][day].TEMPavg)/double(prov[province][year-2000][month][day].counttemp)/10.0;
                        double AH=0.6112*exp(17.67*prov[province][year-2000][month][day].TEMPavg/(prov[province][year-2000][month][day].TEMPavg+243.5))*21.674*prov[province][year-2000][month][day].RHavg/(prov[province][year-2000][month][day].TEMPavg+273.15);
                        double SH=AH/1000.0/airdensity[int(prov[province][year-2000][month][day].TEMPavg+0.5+20)].densitydry;
                        prov[province][year-2000][month][day].adjustR0=DETA*exp(COEF_B*SH);
                        if(prov[province][year-2000][month][day].adjustR0==0){
                            if(month==1 && day==1){
                                prov[province][year-2000][month][day].adjustR0=prov[province][year-2000-1][12][31].adjustR0;
                            }else if(month>1 && day==1){
                                prov[province][year-2000][month][day].adjustR0=prov[province][year-2000][month-1][monthday[month-1]].adjustR0;
                            }else{
                                prov[province][year-2000][month][day].adjustR0=prov[province][year-2000][month][day-1].adjustR0;
                            }
                        }
                    }
                }
                if(year*12+month>=STARTYEAR*12+STARTMONTH && year*12+month<=ENDYEAR*12+ENDMONTH && day<=monthday[month]){
                    if(year*12+month>=STARTYEAR*12+STARTMONTH && year*12+month<=ENDYEAR*12+ENDMONTH && day<=monthday[month]){
                        for (int province=1;province<32;province++){
                            qts_2<<province<<","<<year<<","<<month<<","<<day<<","<<prov[province][year-2000][month][day].RHavg<<","<<prov[province][year-2000][month][day].TEMPavg<<","<<prov[province][year-2000][month][day].adjustR0<<"\n";
                        }
                    }
                }
            }
        }
    }
    qf_2.close();
//    qDebug()<<"end";
//    exit(0);
}

void PatientGenerate(){
    for (int i=0;i<STARTINFECT;i++){
        int randomint=int(PEOPLENUM*QRandomGenerator::global()->generateDouble());
        peo[randomint].infecttime=1+int(i*(LATENTPERIOD+INFECTPERIOD)/STARTINFECT);
        peo[randomint].start=1;
        if(i<STARTINFECT*provpercent[CITYCODE].school){
            peo[randomint].age=1;
        }else if(i<STARTINFECT*provpercent[CITYCODE].school+STARTINFECT*provpercent[CITYCODE].work){
            peo[randomint].age=2;
        }else{
            peo[randomint].age=3;
        }

    }
}

void PeopleStatus(){
    for (int p=0;p<PEOPLENUM;p++){
        double i=QRandomGenerator::global()->generateDouble();
        if(i<provpercent[CITYCODE].school){
            peo[p].age=1;
            peo[p].vulnerability=STUDENTVULNERABILITY;
        }else if(i<provpercent[CITYCODE].school+provpercent[CITYCODE].work){
            peo[p].age=2;
            peo[p].vulnerability=WORKERVULNERABILITY;
        }else{
            peo[p].age=3;
            peo[p].vulnerability=OTHERVULNERABILITY;
        }
    }
}

void HomeAssignment(){
    int linshihome=0;
    for (int p=0;p<PEOPLENUM;p++){
        if(linshihome<homenum){
            peo[p].home=linshihome;
            linshihome++;
        }else{
            peo[p].home=int(homenum*QRandomGenerator::global()->generateDouble());
        }
    }
}

void OfficeAssignment(int companynum, int officenumpercompany){
    for (int p=0;p<PEOPLENUM;p++){
        if(peo[p].age==2){
            peo[p].company=QRandomGenerator::global()->generateDouble()*companynum;
            peo[p].companyoffice=QRandomGenerator::global()->generateDouble()*officenumpercompany;
            if( companyoffice[peo[p].company][peo[p].companyoffice].linshipeonum<OFFICESIZE+2){
                companyofficestep[peo[p].company][ peo[p].companyoffice][companyoffice[peo[p].company][peo[p].companyoffice].linshipeonum].peocode=p;
                companyoffice[peo[p].company][peo[p].companyoffice].linshipeonum+=1;
            }else{
                p=p-1;
            }
        }
    }
    std::default_random_engine generator(time(0));
    std::normal_distribution<double> distribution(OFFICECONNECTPEO,0.15*OFFICECONNECTPEO);
    for (int p=0;p<PEOPLENUM;p++){
        if(peo[p].age==2){
            numofficeconnect= int(distribution(generator)+0.5);
            if (numofficeconnect>OFFICESIZE){
                numofficeconnect=OFFICESIZE;
            }else if(numofficeconnect<0){
                numofficeconnect=0;
            }
            peo[p].officeconnect=numofficeconnect;
            for(int i=0;i<numofficeconnect;i++){
                if(QRandomGenerator::global()->generateDouble()>OFFICEOTHERCONTACTPROB){
                    peoofficefriend[p][i].peocode=companyofficestep[peo[p].company][peo[p].companyoffice][int(QRandomGenerator::global()->generateDouble()*companyoffice[peo[p].company][peo[p].companyoffice].linshipeonum)].peocode;
                }else{
                    int a=int(QRandomGenerator::global()->generateDouble()*officenum);
                    peoofficefriend[p][i].peocode=companyofficestep[peo[p].company][a][int(QRandomGenerator::global()->generateDouble()*companyoffice[peo[p].company][a].linshipeonum)].peocode;
                }
            }
        }
    }

}

void SchoolAssignment(int schoolnum, int classnum){
    for (int p=0;p<PEOPLENUM;p++){
        if(peo[p].age==1){
            peo[p].school=QRandomGenerator::global()->generateDouble()*schoolnum;
            peo[p].schoolclass=QRandomGenerator::global()->generateDouble()*classnum;
            if( schoolclass[peo[p].school][peo[p].schoolclass].linshipeonum<CLASSSIZE+5){
                schoolclassstep[peo[p].school][ peo[p].schoolclass][schoolclass[peo[p].school][peo[p].schoolclass].linshipeonum].peocode=p;
                schoolclass[peo[p].school][peo[p].schoolclass].linshipeonum+=1;
            }else{
                p=p-1;
            }
        }
    }
    std::default_random_engine generator(time(0));
    std::normal_distribution<double> distribution(CLASSCONNECTPEO,0.15*CLASSCONNECTPEO);
    for (int p=0;p<PEOPLENUM;p++){
        if(peo[p].age==1){
            int numclassconnect= int(distribution(generator)+0.5);
            if (numclassconnect>CLASSSIZE){
                numclassconnect=CLASSSIZE;
            }else if(numclassconnect<0){
                numclassconnect=0;
            }
            peo[p].classconnect=numclassconnect;
            for(int i=0;i<numclassconnect;i++){
                if(QRandomGenerator::global()->generateDouble()>SCHOOLOTHERCONTACTPROB){
                    peoclassfriend[p][i].peocode=schoolclassstep[peo[p].school][peo[p].schoolclass][int(QRandomGenerator::global()->generateDouble()*schoolclass[peo[p].school][peo[p].schoolclass].linshipeonum)].peocode;
                }else{
                    int a=int(QRandomGenerator::global()->generateDouble()*classnum);
                    peoclassfriend[p][i].peocode=schoolclassstep[peo[p].school][a][int(QRandomGenerator::global()->generateDouble()*schoolclass[peo[p].school][a].linshipeonum)].peocode;
                }
            }
        }
    }
}

void CommunityAssignment(int communitynum){
    for (int c=0;c<communitynum;c++){
        community[c].recordlist=c;
        community[c].currentpeonum=community[c].peonum;
    }
    int currentcommunitynum=communitynum;
    int linshilinshi=0;
    for (int i=0;i<currentcommunitynum;i++){
        linshilinshi+=community[i].peonum;
    }
    for (int p=0;p<PEOPLENUM;p++){
        int com=int(currentcommunitynum*QRandomGenerator::global()->generateDouble());
        peo[p].community=community[com].recordlist;
        community[community[com].recordlist].currentpeonum--;
        if(community[community[com].recordlist].currentpeonum==0){
            community[com].recordlist=community[currentcommunitynum-1].recordlist;
            currentcommunitynum--;
        }
    }
}

void  HomeInfection(int yyyy,int mm, int d, int t){
#if R0TEST==0
    for (int p=0;p<PEOPLENUM;p++){
        if (t>WORKSTART && t<WORKEND){
            if(peo[p].age==3){
                home[peo[p].home].population++;
                if(peo[p].infecttime>LATENTPERIOD && peo[p].infecttime<=LATENTPERIOD+INFECTPERIOD) {
                    home[peo[p].home].Inum++;
                }
            }
        }else{
            home[peo[p].home].population++;
            if(peo[p].infecttime>LATENTPERIOD && peo[p].infecttime<=LATENTPERIOD+INFECTPERIOD) {
                home[peo[p].home].Inum++;
            }
        }
    }
#endif
#if R0TEST==1
    for (int p=0;p<PEOPLENUM;p++){
        if(peo[p].infecttime>LATENTPERIOD && peo[p].infecttime<=LATENTPERIOD+INFECTPERIOD && peo[p].start==1){
            home[peo[p].home].Inum++;
        }
    }
#endif
    for (int p=0;p<PEOPLENUM;p++){
        if (peo[p].infecttime==0 && home[peo[p].home].Inum>0){
            //            if(QRandomGenerator::global()->generateDouble()<prov[CITYCODE][yyyy-2000][mm][d].adjustR0*BETAHOME*BETACOEF*home[peo[p].home].Inum/pow(home[peo[p].home].population,0.8)){
            if(QRandomGenerator::global()->generateDouble()<prov[CITYCODE][yyyy-2000][mm][d].adjustR0*BETAHOME*peo[p].vulnerability*BETACOEF*home[peo[p].home].Inum){
                peo[p].infecttime=1;
                stat.homeinfect++;
                if(peo[p].age==1){
                    stat.studentinfect++;
                    stat.studenthome++;
                }else if(peo[p].age==2){
                    stat.workerinfect++;
                    stat.workerhome++;
                }else{
                    stat.unemployedinfect++;
                    stat.unemployedhome++;
                }
            }
        }
    }
}

void CommunityInfection(int yyyy, int mm, int d){
#if R0TEST==0
    for (int p=0;p<PEOPLENUM;p++){
        if(peo[p].infecttime>LATENTPERIOD && peo[p].infecttime<=LATENTPERIOD+INFECTPERIOD){
            community[peo[p].community].Inum++;
        }
    }
#endif
#if R0TEST==1
    for (int p=0;p<PEOPLENUM;p++){
        if(peo[p].infecttime>LATENTPERIOD && peo[p].infecttime<=LATENTPERIOD+INFECTPERIOD){
            community[peo[p].community].Inum++;
        }
    }
#endif
    for (int p=0;p<PEOPLENUM;p++){
        if (peo[p].infecttime==0){
            if(community[peo[p].community].Inum>0){
                if(QRandomGenerator::global()->generateDouble()<prov[CITYCODE][yyyy-2000][mm][d].adjustR0*BETACOMMUNITY*peo[p].vulnerability*BETACOEF*community[peo[p].community].Inum){
                    peo[p].infecttime=1;
                    stat.communityinfect++;
                    if(peo[p].age==1){
                        stat.studentcommunity++;
                        stat.studentinfect++;
                    }else if(peo[p].age==2){
                        stat.workerinfect++;
                        stat.workercommunity++;
                    }else{
                        stat.unemployedinfect++;
                        stat.unemployedcommunity++;
                    }
                }
            }
        }
    }
}

void OfficeInfection(int yyyy, int mm, int d){
    for (int p=0;p<PEOPLENUM;p++){
        if(peo[p].age==2 && peo[p].infecttime==0){
            for(int i=0;i<peo[p].officeconnect;i++){
                if(peo[peoofficefriend[p][i].peocode].infecttime>LATENTPERIOD && peo[peoofficefriend[p][i].peocode].infecttime<=LATENTPERIOD+INFECTPERIOD){
                    if(QRandomGenerator::global()->generateDouble()<prov[CITYCODE][yyyy-2000][mm][d].adjustR0*peo[p].vulnerability*BETAOFFICE*BETACOEF){
                        peo[p].infecttime=1;
                        stat.officeinfect++;
                        stat.workerinfect++;
                        stat.workeroffice++;
                    }
                }
            }
        }
    }
}

void SchoolInfection(int yyyy, int mm, int d){
    for (int p=0;p<PEOPLENUM;p++){
        if(peo[p].age==1 && peo[p].infecttime==0){
            for(int i=0;i<peo[p].classconnect;i++){
                if(peo[peoclassfriend[p][i].peocode].infecttime>LATENTPERIOD && peo[peoclassfriend[p][i].peocode].infecttime<=LATENTPERIOD+INFECTPERIOD){
                    if(QRandomGenerator::global()->generateDouble()<prov[CITYCODE][yyyy-2000][mm][d].adjustR0*peo[p].vulnerability*BETASCHOOL*BETACOEF){
                        peo[p].infecttime=1;
                        stat.schoolinfect++;
                        stat.studentinfect++;
                        stat.studentschool++;
                    }
                }
            }
        }
    }
}

void statistics(int yyyy,int mm, int dd){
    for(int p=0;p<PEOPLENUM;p++){
        if(peo[p].infecttime>0 && peo[p].infecttime<=24){
            summary[totalstep][biground].Inum++;
        }else if(peo[p].infecttime>0 && peo[p].infecttime<=LATENTPERIOD+INFECTPERIOD){
            stat.Inum++;
            summary[totalstep][biground].Snum++;
        }else if(peo[p].infecttime>LATENTPERIOD+INFECTPERIOD){
            summary[totalstep][biground].Rnum++;
        }
    }
#if TARGET==1
    qDebug()<<CITYCODE<<","<<DETA<<","<<biground<<","<<yyyy<<","<<mm<<","<<dd<<","<<stat.homeinfect+stat.officeinfect+stat.communityinfect+stat.schoolinfect;
    if(stat.Inum==0){
        validnum[biground].homeinfect=stat.homeinfect;
        validnum[biground].officeinfect=stat.officeinfect;
        validnum[biground].schoolinfect=stat.schoolinfect;
        validnum[biground].communityinfect=stat.communityinfect;
    }else if(yyyy==ENDYEAR && mm==ENDMONTH && dd==30){
        validnum[biground].homeinfect=stat.homeinfect;
        validnum[biground].officeinfect=stat.officeinfect;
        validnum[biground].schoolinfect=stat.schoolinfect;
        validnum[biground].communityinfect=stat.communityinfect;
    }
#elif TARGET==2
    qDebug()<<URBANRATE<<","<<biground<<","<<yyyy<<","<<mm<<","<<dd<<","<<(stat.homeinfect+stat.officeinfect+stat.communityinfect+stat.schoolinfect)<<","<<stat.studenthome<<","<<stat.studentschool<<","<<stat.studentcommunity<<","<<stat.workerhome<<","<<stat.workeroffice<<","<<stat.workercommunity<<","<<stat.unemployedhome<<","<<stat.unemployedcommunity;
    if(stat.Inum==0){
        validnum[biground].homeinfect=stat.homeinfect;
        validnum[biground].officeinfect=stat.officeinfect;
        validnum[biground].schoolinfect=stat.schoolinfect;
        validnum[biground].communityinfect=stat.communityinfect;
    }else if(yyyy==ENDYEAR && mm==ENDMONTH && dd==30){
        validnum[biground].homeinfect=stat.homeinfect;
        validnum[biground].officeinfect=stat.officeinfect;
        validnum[biground].schoolinfect=stat.schoolinfect;
        validnum[biground].communityinfect=stat.communityinfect;
    }
#endif
}

void clean(int company, int schoolnum){
    stat.Snum=0;
    stat.Inum=0;
    stat.Rnum=0;
    for (int i=0;i<homenum;i++){
        home[i].Snum=0;
        home[i].Inum=0;
        home[i].Rnum=0;
        home[i].population=0;
    }
    for (int i=0;i<communitynum;i++){
        community[i].Snum=0;
        community[i].Inum=0;
        community[i].Rnum=0;
    }
    for (int i=0;i<company;i++){
        office[i].Snum=0;
        office[i].Inum=0;
        office[i].Rnum=0;
    }
    for (int i=0;i<schoolnum;i++){
        school[i].Snum=0;
        school[i].Inum=0;
        school[i].Rnum=0;
    }
}
void RoundClean(int round, int officenum, int schoolnum){
    stat.Snum=0;
    stat.Inum=0;
    stat.Rnum=0;
    for (int i=0;i<homenum;i++){
        home[i].Snum=0;
        home[i].Inum=0;
        home[i].Rnum=0;
        home[i].population=0;
    }
    for (int i=0;i<communitynum;i++){
        community[i].Snum=0;
        community[i].Inum=0;
        community[i].Rnum=0;
    }
    for (int i=0;i<officenum;i++){
        office[i].Snum=0;
        office[i].Inum=0;
        office[i].Rnum=0;
        for(int j=0;j<(COMPANYSIZE-1)/OFFICESIZE+1;j++){
            companyoffice[i][j].linshipeonum=0;
            for(int k=0;k<OFFICESIZE+2;k++){
                companyofficestep[i][j][k].peocode=0;
            }
        }
    }
    for (int i=0;i<schoolnum;i++){
        school[i].Snum=0;
        school[i].Inum=0;
        school[i].Rnum=0;
        for(int j=0;j<provpercent[CITYCODE].schoolsize/CLASSSIZE+1;j++){
            schoolclass[i][j].linshipeonum=0;
            for(int k=0;k<CLASSSIZE+5;k++){
                schoolclassstep[i][j][k].peocode=0;
            }
        }
    }
    for (int p=0;p<PEOPLENUM;p++){
        peo[p].start=0;
        peo[p].company=0;
        peo[p].companyoffice=0;
        peo[p].school=0;
        peo[p].schoolclass=0;
        peo[p].community=0;
        peo[p].home=0;
        peo[p].infecttime=0;
        peo[p].officeconnect=0;
        peo[p].classconnect=0;
    }
    if(round<ROUND-1){
        totalstep=0;
    }
}


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
#if RANDOMSEED==0
    qsrand(0);
#else
    qsrand(QTime::currentTime().msecsSinceStartOfDay());
#endif
#if R0TEST==0
    QFile qf_1("C:/Users/stoja/Desktop/Infection.csv");
    QTextStream qts_1(&qf_1);
    qf_1.open(QIODevice::WriteOnly|QIODevice::Text);
#endif
    ClimateDataLoad();                 //读取气象数
#if TARGET==2
    provpercent[CITYCODE].housesize=3.997-1.7*URBANRATE;
    provpercent[CITYCODE].school=0.29195-0.226*URBANRATE;
    provpercent[CITYCODE].work=0.51831+0.178*URBANRATE;
    provpercent[CITYCODE].schoolsize=1360.5*URBANRATE+75.148+0.5;
    communitysize=int(0.5+COMMUNITYSIZE*pow(1.056,URBANRATE*100)/pow(1.056,86));
#endif
    homenum=PEOPLENUM/provpercent[CITYCODE].housesize;
    std::default_random_engine generator(time(0));
    std::normal_distribution<double> distribution(communitysize,0.15*communitysize);
    int linshicommunitynum=0;
    for (int i=0;i<PEOPLENUM;i++){
        int communitypeonum= int(distribution(generator)+0.5);
        if(communitypeonum<1){
            communitypeonum=1;
        }
        community[i].peonum=communitypeonum;
        linshicommunitynum+=communitypeonum;
        communitynum++;
        if(linshicommunitynum>PEOPLENUM){
            break;
        }
    }
    int companynum=(PEOPLENUM*provpercent[CITYCODE].work-1)/COMPANYSIZE+1;
    int officenumpercompany=(COMPANYSIZE-1)/OFFICESIZE+1;
    int schoolnum=(PEOPLENUM*provpercent[CITYCODE].school-1)/provpercent[CITYCODE].schoolsize+1;
    int classnumperschool=(provpercent[CITYCODE].schoolsize-1)/CLASSSIZE+1;
    for (int round=0;round<ROUND;round++){
        int programstop=0;            //如果等于1就停止了。
        biground=round;
        PeopleStatus();             //分配人员属性,工作和上学的比例
        PatientGenerate();          //设定初始感染者
        HomeAssignment();           //给大家分家
        OfficeAssignment(companynum,officenumpercompany);         //给大家分办公室
        SchoolAssignment(schoolnum,classnumperschool);         //给大家分配学校
        int yyyy=STARTYEAR;
        for (int m=STARTMONTH; m<12*(ENDYEAR-STARTYEAR)+ENDMONTH+1;m++){
            if(programstop==1){
                break;
            }
            int dd;
            int mm=(m-1)%12+1;
            if(mm==2){
                if(QDate::isLeapYear(yyyy)){
                    dd=29;
                }else{
                    dd=28;
                }
            }else if(mm==1 || mm==3 || mm==5 || mm==7 || mm==8 || mm==10 || mm==12){
                dd=31;
            }else{
                dd=30;
            }
            for (int d=1;d<dd+1;d++){
                CommunityAssignment(communitynum);
                totalstep++;
                for (int t=0;t<24;t++){
                    for (int p=0;p<PEOPLENUM;p++){
                        if(peo[p].infecttime>0 && peo[p].infecttime<RECOVERPERIOD+LATENTPERIOD+INFECTPERIOD){
                            peo[p].infecttime++;
                        }else if(peo[p].infecttime==RECOVERPERIOD+LATENTPERIOD+INFECTPERIOD){
                            peo[p].infecttime=0;
                        }
                    }
                    if(t%24<=HOMELEAVE){
//                        HomeInfection(yyyy,mm,d,t);
                    }else if(t%24>HOMELEAVE && t%24<=WORKSTART){
                        CommunityInfection(yyyy,mm,d);
                    }else if(t%24>WORKSTART && t%24<=WORKEND){
                        OfficeInfection(yyyy,mm,d);
                        SchoolInfection(yyyy,mm,d);
                        HomeInfection(yyyy,mm,d,t);
                    }else if (t%24>WORKEND && t%24<=HOMEBACK){
                        CommunityInfection(yyyy,mm,d);
                    }else{
                        HomeInfection(yyyy,mm,d,t);
                    }
                    clean(companynum, schoolnum);
                }
                if (TARGET==1){
                    if(yyyy==STARTYEAR && mm==STARTMONTH && d==1){
                        qDebug()<<"province,Deta,round,year,month,day,totalinfect";
                    }
                }else{
                    if(yyyy==STARTYEAR && mm==STARTMONTH && d==1){
                        qDebug()<<"urbanization,round,year,month,day,total,studenthome,studentschool,studentcommunity,workerhome,workeroffice,workercommunity,unemployedhome,unemployedcommunity";
                    }
                }
                statistics(yyyy,mm,d);
                if (stat.Inum==0){
                    programstop=1;
                    break;
                }
                if(mm%12==0 && d==31){
                    yyyy++;
                }
            }
        }
        RoundClean(round, companynum, schoolnum);
    }
    for (int round=ROUND-1;round>0;round--){
        validnum[round].homeinfect-=validnum[round-1].homeinfect;
        validnum[round].officeinfect-=validnum[round-1].officeinfect;
        validnum[round].schoolinfect-=validnum[round-1].schoolinfect;
        validnum[round].communityinfect-=validnum[round-1].communityinfect;
    }
    qDebug()<<"round,home,community,office,school,total";
    for (int round=0;round<ROUND;round++){
        int totalinfected=validnum[round].homeinfect+validnum[round].communityinfect+validnum[round].officeinfect+validnum[round].schoolinfect;
        qDebug()<<round<<","<<validnum[round].homeinfect<<","<<validnum[round].communityinfect<<","<<validnum[round].officeinfect<<","<<validnum[round].schoolinfect<<","<<totalinfected;
    }
#if R0TEST==0
    for (int round=0;round<ROUND;round++){
        int maxvalue=0;
        int maxstep=0;
        for(int i=0;i<totalstep;i++){
            if(summary[i][round].Inum>maxvalue){
                maxvalue=summary[i][round].Inum;
                maxstep=i;
            }
        }
        summary[0][round].peakday=maxstep;
        if (maxstep<200){
            for(int i=400;i>200-maxstep-1;i--){
                summary[i][round].Inum=summary[i-200+maxstep][round].Inum;
                summary[i][round].Snum=summary[i-200+maxstep][round].Snum;
            }
            for (int i=200-maxstep-1;i>-1;i--){
                summary[i][round].Inum=0;
                summary[i][round].Snum=0;
            }
        }else{
            for(int i=0;i<400-maxstep+1;i++){
                summary[i][round].Inum=summary[i-200+maxstep][round].Inum;
                summary[i][round].Snum=summary[i-200+maxstep][round].Snum;
            }
            for(int i=400-maxstep+1;i<401;i++){
                summary[i][round].Inum=0;
                summary[i][round].Snum=0;
            }
        }
        if (maxvalue<0.0001*PEOPLENUM){
            validnum[round].valid=1;
        }
    }
    int validnumber=0;
    for (int round=0;round<ROUND;round++){
        if(validnum[round].valid==0){
            validnumber++;
            for (int i=0;i<401;i++){
                if(validnum[round].valid==0){
                    summary[i][0].totalInum+=summary[i][round].Inum;
                    summary[i][0].totalSnum+=summary[i][round].Snum;
                }
            }
        }
    }
    for (int i=0;i<401;i++){
        summary[i][0].totalInum=summary[i][0].totalInum/validnumber;
        summary[i][0].totalSnum=summary[i][0].totalSnum/validnumber;
        qts_1<<i<<","<<summary[i][0].totalInum/double(PEOPLENUM)<<","<<summary[i][0].totalSnum/double(PEOPLENUM);
        for (int round=0;round<ROUND;round++){
            if(validnum[round].valid==0){
                qts_1<<","<<summary[i][round].Inum/double(PEOPLENUM);
            }
        }
        qts_1<<"\n";
    }
#endif
#if R0TEST==0
    qf_1.close();
#endif

    qInfo("end");
    return 0;
}
