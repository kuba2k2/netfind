cmake_minimum_required(VERSION 3.20)

project(lt_logger)

add_library(lt_logger STATIC lt_logger.c)
target_include_directories(lt_logger PUBLIC ".")
