#include <iostream>
#include <string>


/* 提供一个本地的简单服务*/
class UserService{
    UserService() = default; 
    void login(std::string name, std::string pwd){
        std::cout << "Doing the local service: login..."<< std::endl; 
        std::cout << "Name: " << name << " pwd: " << pwd << std::endl; 
    }
}; 
