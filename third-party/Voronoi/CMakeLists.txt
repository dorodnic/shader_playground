cmake_minimum_required(VERSION 2.8.3)

project(voronoi)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(voronoi
    src/BeachLine.cpp
	src/Cell.cpp
	src/CircleEventQueue.cpp
	src/Diagram.cpp
	src/Edge.cpp
	src/Point2.cpp
	src/Vector2.cpp
	src/VoronoiDiagramGenerator.cpp
    )