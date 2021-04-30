//Program de testare a performantelor de transfer pentru memoriile cache
/* Se vor evidentia vitezele de transfer
pentru date aflate in memoria cache, in
memoria interna si in memoria virtuala
Se vor trasa grafice in care se indica
dependenta vitezei de transfer in raport
diferiti parametri (ex: dimensiunea
blocurilor transferate) (Java, C, C++, C#
etc.)*/

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <random>
#include <cstdlib>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <mm_malloc.h>
#include <sys/sendfile.h>
#include <sys/mman.h>
#include <xmmintrin.h>
#include <bits/stdc++.h> 
#include <chrono> 



int megabyte = 1024 * 1024;
int dataSize = 4096; //4 KB
int count = 4;
int fileSize = count * dataSize;

char* cached_data;
int fd_dest1;
int fd_dest;
int fd_src;
int fd;

int shm_fd;
char* shared_buf = NULL;
size_t shm_size = dataSize;




int main() {

    srand(time(NULL));

    FILE* dataTable = fopen("dataTable.csv", "w+");
    fprintf(dataTable, "%s,,%s,,%s,,%s\n","File size", "Cache memory", "Virtual Memory", "Physical Memory");


    for(int k = 0; k < 5; k++){
        double transfer_time = 0;
        double transfer_speed;
        remove("data.txt");
        remove("data_dest.txt");
        remove("data_dest1.txt");
        fileSize = count * dataSize;
        fprintf(dataTable, "%d KB,,", fileSize);

        //CACHE MEMORY
        char* dummy = (char*)_mm_malloc(sizeof(char), 4096);
        cached_data = (char*)_mm_malloc(sizeof(char) * dataSize, 4096);
        for(int i = 0; i < dataSize; i++)
            cached_data[i] = rand() % 256;

        fd_dest1 = open("data_dest1.txt", O_CREAT | O_RDWR | O_APPEND | O_DIRECT, 0666);

        _mm_prefetch(cached_data, _MM_HINT_T0);
        for(int i = 0; i < 1000000; i++)
            read(fd_dest1, dummy, 1);


        for(int i = 0; i < 3; i++){
            auto start = std::chrono::high_resolution_clock::now(); 
            std::ios_base::sync_with_stdio(false);

            for(int i = 0; i < count; i++)
                write(fd_dest1, cached_data, dataSize);
            
            auto end = std::chrono::high_resolution_clock::now(); 
            transfer_time += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        }
        transfer_time /= 3000000000.0;
        transfer_speed = fileSize * 1.0 / transfer_time / 1024 / 1024;
        printf("CACHE MEMORY\n");
        printf("transfer time : %f seconds\ntransfer speed : %f MB/s\nfile size : %d KB\n", transfer_time, transfer_speed, fileSize / 1024);
        fprintf(dataTable, "%f MB/s,,", transfer_speed);

        system("sudo insmod disable_cache.ko");

        //Virtual Memory
        shm_fd = shm_open("shm", O_RDWR | O_CREAT, 0600);
        if(shm_fd < 0) {
            perror("Could not aquire shm");
            return 1;
        }
        ftruncate(shm_fd, shm_size * sizeof(char));
        shared_buf = (char*)mmap(0, shm_size, PROT_READ |PROT_WRITE, MAP_PRIVATE, shm_fd, 0);
        if(shared_buf == MAP_FAILED){
            perror("Could not map the shared memory");
            close(shm_fd);
            return 1;
        }
        volatile char* volatile_buf = shared_buf;
        for(int i = 0; i < shm_size; i++){
            volatile_buf[i] = cached_data[i];
        }
        fd = open("data.txt", O_CREAT | O_RDWR | O_DIRECT | O_APPEND | O_SYNC, 0666);
        transfer_time = 0;
        for(int i = 0; i < 3; i++){
            auto start = std::chrono::high_resolution_clock::now(); 
            std::ios_base::sync_with_stdio(false);

            for(int i = 0; i < count; i++)
                write(fd, (const void*)volatile_buf, dataSize);

            auto end = std::chrono::high_resolution_clock::now();
            transfer_time += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        }
        transfer_time /= 3000000000.0;
        transfer_speed = fileSize * 1.0 / transfer_time / 1024 / 1024;

        printf("VIRTUAL MEMORY\n");
        printf("transfer time : %f seconds\ntransfer speed : %f MB/s\nfile size : %d KB\n", transfer_time, transfer_speed, fileSize / 1024);
        fprintf(dataTable, "%f MB/s,,", transfer_speed);

        //Physical Memory
        fd_src = open("data.txt", O_RDONLY | O_DIRECT | O_SYNC, 0666);
        fd_dest = open("data_dest.txt", O_CREAT | O_WRONLY | O_DIRECT | O_SYNC, 0666);
        struct stat inode;
        fstat(fd_src, &inode);
        char* buf = (char*)_mm_malloc(dataSize * 1, 4096);;

        transfer_time = 0;
        for(int i = 0; i < 3; i++){
            auto start = std::chrono::high_resolution_clock::now(); 
            std::ios_base::sync_with_stdio(false);
            for(int i = 0; i < count; i++){
                read(fd_src, buf, dataSize);
                write(fd_dest, buf, dataSize);
            }    

            auto end = std::chrono::high_resolution_clock::now(); 
            transfer_time += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        }

        transfer_time /= 3000000000.0;
        transfer_speed = fileSize * 1.0 / 1024 / 1024 / transfer_time;
        printf("PHYSICAL MEMORY\n");
        printf("transfer time : %f seconds\ntransfer speed : %f MB/s\nfile size : %d KB\n", transfer_time, transfer_speed, fileSize / 1024);
        fprintf(dataTable, "%f MB/s\n", transfer_speed);
        
        count = count * 10;

        system("sudo rmmod disable_cache.ko");
        close(fd_src);
        close(fd);
        close(fd_dest);
        close(fd_dest1);
        munmap(shared_buf, shm_size);
        shared_buf = NULL;
        shm_unlink("shm");
        _mm_free(cached_data);

    }
    fclose(dataTable);
    remove("data.txt");
    remove("data_dest.txt");
    remove("data_dest1.txt");
}