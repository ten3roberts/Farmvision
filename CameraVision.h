#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/v4l2-common.h>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fstream>
#include <string>

class Camera
{
public:
    //File Descriptor
    int fd;
    // 2. Ask the device if it can capture frames
    v4l2_capability capability;

    // 3. Set Image format
    v4l2_format imageFormat;

    // 4. Request Buffers from the device
    v4l2_requestbuffers requestBuffer;
    v4l2_buffer queryBuffer;
    char* buffer;
    v4l2_buffer bufferinfo;
    int type = bufferinfo.type;

public:
    int Open()
    {
        std::cout << "Camera Initialization: " << std::endl;

        fd = open("/dev/video0",O_RDWR);
        if(fd < 0)
        {
            perror("Failed to open device, OPEN");
            return 0;
        }
        std::cout << "Opened camera" << std::endl;

        // 2. Ask the device if it can capture frames
        if(ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0)
        {
            // something went wrong... exit
            perror("Failed to get camera capabilities, VIDIOC_QUERYCAP");
            return 0;
        }

        std::cout << "Got camera capabilities" << std::endl;
        // 3. Set Image format
        imageFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        imageFormat.fmt.pix.width = 1280;
        imageFormat.fmt.pix.height = 720;
        imageFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        imageFormat.fmt.pix.field = V4L2_FIELD_NONE;
        // tell the device you are using this format
        if(ioctl(fd, VIDIOC_S_FMT, &imageFormat) < 0)
        {
            perror("Camera could not set format, VIDIOC_S_FMT");
            return 0;
        }
        std::cout << "Set camera format" << std::endl;

        // 4. Request Buffers from the device
        requestBuffer = {0};
        requestBuffer.count = 1; // one request buffer
        requestBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // request a buffer wich we an use for capturing frames
        requestBuffer.memory = V4L2_MEMORY_MMAP;

        if(ioctl(fd, VIDIOC_REQBUFS, &requestBuffer) < 0)
        {
            perror("Could not request buffer from camera, VIDIOC_REQBUFS");
            return 0;
        }

        std::cout << "Requested camera buffer" << std::endl;
        // 5. Query the buffer to get raw data ie. ask for the you requested buffer
        // and allocate memory for it
        queryBuffer = {0};
        queryBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        queryBuffer.memory = V4L2_MEMORY_MMAP;
        queryBuffer.index = 0;
        if(ioctl(fd, VIDIOC_QUERYBUF, &queryBuffer) < 0)
        {
            perror("Camera did not return the buffer information, VIDIOC_QUERYBUF");
            return 0;
        }

        std::cout << "Queried camera buffers" << std::endl;

        // use a pointer to point to the newly created buffer
        // mmap() will map the memory address of the device to
        // an address in memory
        buffer = (char*)mmap(NULL, queryBuffer.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                             fd, queryBuffer.m.offset);
        memset(buffer, 0, queryBuffer.length);


        // 6. Get a frame
        // Create a new buffer type so the device knows whichbuffer we are talking about
        memset(&bufferinfo, 0, sizeof(bufferinfo));
        bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        bufferinfo.memory = V4L2_MEMORY_MMAP;
        bufferinfo.index = 0;

        // Activate streaming
        type = bufferinfo.type;
        if(ioctl(fd, VIDIOC_STREAMON, &type) < 0)
        {
            perror("Could not start streaming, VIDIOC_STREAMON");
            return 0;
        }
        std::cout << "Succesfully opened camera" << std::endl;
        return 1;

    }
    int Capture(std::string saveFile)
    {
        /***************************** Begin looping here *********************/
        // Queue the buffer
        if(ioctl(fd, VIDIOC_QBUF, &bufferinfo) < 0)
        {
            perror("Could not queue buffer, VIDIOC_QBUF");
            return 0;
        }

        // Dequeue the buffer
        if(ioctl(fd, VIDIOC_DQBUF, &bufferinfo) < 0)
        {
            perror("Could not dequeue the buffer, VIDIOC_DQBUF");
            return 0;
        }
        // Frames get written after dequeuing the buffer

        std::cout << "Camera buffer has: " << (double)bufferinfo.bytesused / 1024
                  << " KBytes of data" << std::endl;


        std::cout << "Saving camera stream to: " << saveFile << "..." << std::endl;
        // Open the file
        std::ofstream outFile;
        outFile.open(saveFile, std::ios::binary| std::ios::trunc);

        // Write the data out to file
        outFile.write(buffer, (double)bufferinfo.bytesused);

        // Close the file
        outFile.close();

        return 1;
    }
    int Close()
    {
        // end streaming
        if(ioctl(fd, VIDIOC_STREAMOFF, &type) < 0)
        {
            perror("Could not end streaming, VIDIOC_STREAMOFF");
            return 0;
        }

        close(fd);
        std::cout << "Closed feed" << std::endl;
        return 0;
    }
};
