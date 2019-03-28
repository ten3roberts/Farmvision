#include "CameraVision.h"
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <sstream>
#include <ctime>
#include <time.h>
#include <chrono>

//using namespace std;
using namespace std::chrono_literals;

#define MILLION 1000000
#define BILLION 1000000000

#define MINUTE 60
#define HOUR 3600
#define DAY 86400


class Time
{
public:
    static void Init()
    {
        startTime = std::chrono::steady_clock::now();
        startDateAndTime = GetDateAndTime();
    }
    static void Update()
    {
        frameCount++;

        currentTime = std::chrono::steady_clock::now();
        elapsedTime = (currentTime - startTime).count();

        deltaTime = (currentTime - prevTime).count() / (double)BILLION;

        frameRate = 1.0f / deltaTime;

        prevTime = currentTime;
    }
    static std::string GetDateAndTime()
    {
        std::stringstream date;
        time_t t = time(0);   // get time now
        struct tm * now = localtime( & t );
        date << (now->tm_year + 1900) << '-'
             << (now->tm_mon + 1) << '-'
             <<  now->tm_mday << "_"
             << now->tm_hour << "."
             << now->tm_min;

        return date.str();
    }
    static double frameRate;
    static double deltaTime;
    static double elapsedTime;
    static std::string startDateAndTime;
    static int frameCount;

    static std::chrono::time_point<std::chrono::steady_clock> startTime;
    static std::chrono::time_point<std::chrono::steady_clock> prevTime;
    static std::chrono::time_point<std::chrono::steady_clock> currentTime;
};



double Time::frameRate = 1.0;
double Time::deltaTime = 1.0;
double Time::elapsedTime = 0.0;
std::string Time::startDateAndTime = "";
int Time::frameCount = 0;
std::chrono::time_point<std::chrono::steady_clock> Time::startTime = std::chrono::steady_clock::now();
std::chrono::time_point<std::chrono::steady_clock> Time::prevTime = std::chrono::steady_clock::now();
std::chrono::time_point<std::chrono::steady_clock> Time::currentTime = std::chrono::steady_clock::now();

enum TimeAlign
{
    ALIGN_FREE = 1, //Will take pictures as soon as the prigramn starts
    ALIGN_MINUTE = 60, //Will take pictures aligned on minutes
    ALIGN_HOUR = 3600, //Will take pictures aligned on hours
    ALIGN_DAY = 86400 //Will take pictures aligned at noon
};

struct Config
{
    Config()
    {

        alignment = ALIGN_FREE;
        imageInterval = 1;
    }

    bool LoadFromArgs(int argc, char** argv)
    {
        std::cout << "Config: " << std::endl;
        if(argc < 4)
        {
            std::cout << "Too few arguments\r\nUsage: ImageDest TimeAlignment ImageInterval" << std::endl;
            std::cout << "Using config file" << std::endl;
            LoadFromFile("./Config.txt");
            return false;
        }
        //Output directory
        outputDir = argv[1];

        //Make sure the output dir has a leading slash to make it into a path
        if(outputDir[outputDir.size()-1] != '/')
            outputDir += "/";

        std::string alignArg = argv[2];
        alignment = alignArg == "MINUTE" ? ALIGN_MINUTE : alignArg == "HOUR" ? ALIGN_HOUR : alignArg == "DAY" ? ALIGN_DAY : ALIGN_FREE;
        imageInterval = std::stof(argv[3]) * alignment;

        std::cout << "OutputDir: " << outputDir << std::endl;
        std::cout << "Alignment: " << alignment << std::endl;
        std::cout << "Image interval: " << imageInterval << std::endl;
        return true;

    }

    bool LoadFromFile(std::string path)
    {
        outputDir = "./";
        alignment = ALIGN_MINUTE;
        imageInterval = 1;


        std::ifstream file(path);

        if (!file.is_open())
        {
            std::cout << "Couldn't locate config file. Reverting to default settings" << std::endl;
            return false;
        }
        while (file.good())
        {
            std::string line;
            std::getline(file, line);


            if (line.find("OutputDir: ") != std::string::npos)
            {
                std::string value = line.substr(line.find("OutputDir: ") + 11);
                outputDir = value;

            }

            if (line.find("Align:") != std::string::npos)
            {
                std::string value = line.substr(line.find("Align: ") + 7);
                //std::cout << "Align: " << value << std::endl;
                alignment = value == "MINUTE" ? ALIGN_MINUTE : value == "HOUR" ? ALIGN_HOUR : value == "DAY" ? ALIGN_DAY : ALIGN_FREE;

            }
            if (line.find("Interval: ") != std::string::npos)
            {
                std::string value = line.substr(line.find("Interval: ") + 10);
                //std::cout << "Interval: " << value << std::endl;

                imageInterval = stof(value) * alignment;

                //Make sure the output dir has a leading slash to make it into a path
                if(outputDir[outputDir.size()-1] != '/')
                    outputDir += "/";
            }
        }
        std::cout << "OutputDir: " << outputDir << std::endl;
        std::cout << "Alignment: " << alignment << std::endl;
        std::cout << "Image interval: " << imageInterval << std::endl;

        std::cout << "OutputDir: " << outputDir << std::endl;
        std::cout << "Alignment: " << alignment << std::endl;
        std::cout << "Image interval: " << imageInterval << std::endl;

        return true;
    }



    TimeAlign alignment;
    float imageInterval; //The time between images in seconds
    std::string outputDir;
};
int main(int argc, char** argv)
{
    Config config;
    //config.LoadFromFile("./Config.txt");
    config.LoadFromArgs(argc, argv);


    float lastImage = 0;//config.imageInterval;

    Time::Init();


    Camera cam;
    cam.Open();

    while (true)
    {
        Time::Update();

        lastImage += Time::deltaTime;

        float timeToImage = (config.imageInterval - lastImage);
        float timeToMinute = (MINUTE - (time(NULL)) % MINUTE);
        float timeToHour = (HOUR - (time(NULL)) % HOUR);
        float timeToDay = (DAY - (time(NULL)) % DAY) + HOUR * 12; //Time to midnight + time from midnight to noon


        float timeToAlign =
            config.alignment == ALIGN_MINUTE ? timeToMinute :
            config.alignment == ALIGN_HOUR ? timeToHour :
            config.alignment == ALIGN_DAY ? timeToDay : timeToImage;

        std::cout << "\r\nNext minute in " << timeToMinute << " seconds" << std::endl;
        std::cout << "Next hour in " << timeToHour << " seconds" << std::endl;
        std::cout << "Next day in " << timeToDay << " seconds\r\n" << std::endl;

        float sleepDuration = 0;


        //If it's less than one second to the next minute/hour/day don't sleep as you could easily overshoot
        if(abs(timeToAlign) > 10)
            sleepDuration = std::max(timeToAlign, timeToImage);
        else
            sleepDuration = timeToImage;
        if (timeToAlign > 2 && timeToAlign < 58)
        {
            std::cout << "Sleeping for " << sleepDuration << " seconds" << std::endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(((int)sleepDuration) * 1000));
        }

        if (lastImage < config.imageInterval)
            continue;
        lastImage = 0;


        //It's time to take an image
        std::cout << "Taking picture, smile!" << std::endl;

        //Saving the image
        cam.Capture(config.outputDir + Time::GetDateAndTime() + ".jpg");

        std::cout << "\r\n-------------------------------------------------------------" << std::endl;
    }

    cam.Close();

    return 0;
}
