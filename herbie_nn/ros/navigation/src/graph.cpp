//6/16/19 This file contains functions for the graph map

#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <std_msgs/Float64MultiArray.h>
#include <geometry_msgs/Twist.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <image_transport/image_transport.h>
#include <ros/console.h>
#include <iostream>
#include <boost/thread.hpp>
#include <cmath>
#include "navigation.h"

#include <lemon/list_graph.h>
#include <unordered_map>

using namespace std;

typedef lemon::ListGraph Graph;
typedef Graph::NodeIt NodeIt;
typedef Graph::Node Node;
typedef Graph::EdgeIt EdgeIt;
typedef Graph::Edge Edge;

void Navigation::graph_init() {
   std::unordered_map<std::string, lemon::ListGraph::Node> name2node;
   lemon::ListGraph::NodeMap<std::string> name(main_map);
   lemon::ListGraph::NodeMap<double> nodetype(main_map);

   std::vector<std::string> straight_nodes {
      "1_2_straight",  //0   
      "2_1_straight",  //1 

      "2_3_straight",  //2 
      "3_2_straight",  //3 

      "2_4_straight",  //4 
      "4_2_straight",  //5 

      "5_4_straight",  //6 
      "4_5_straight",  //7 

      "5_7_straight",  //8 
      "7_5_straight",  //9 

      "6_7_straight",  //10 
      "7_6_straight",  //11 

      "6_9_straight",  //12 
      "9_6_straight",  //13 

      "8_9_straight",  //14 
      "9_8_straight",  //15 

      "8_1_straight",  //16 
      "1_8_straight",  //17 

      "9_10_straight", //18 
      "10_9_straight", //19 

      "11_10_straight", //20
      "10_11_straight", //21

      "11_3_straight",  //22
      "3_11_straight",  //23

      "10_13_straight", //24
      "13_10_straight", //25

      "12_13_straight", //26
      "13_12_straight", //27

      "12_8_straight",  //28
      "8_12_straight"}; //29


/*  def add_straight_nodes(self): */
/*         #naming convention: start_finish */
/*         self.straight_nodes = [ */
/*             "1_2_straight",   # 0 */
/*             "2_1_straight",   # 1 */

/*             "2_3_straight",   # 2 */
/*             "3_2_straight",   # 3 */

/*             "2_4_straight",   # 4 */
/*             "4_2_straight",   # 5 */

/*             "5_4_straight",   # 6 */
/*             "4_5_straight",   # 7 */

/*             "5_7_straight",   # 8 */
/*             "7_5_straight",   # 9 */

/*             "6_7_straight",   # 10 */
/*             "7_6_straight",   # 11 */

/*             "6_9_straight",   # 12 */
/*             "9_6_straight",   # 13 */

/*             "8_9_straight",   # 14 */
/*             "9_8_straight",   # 15 */

/*             "8_1_straight",   # 16 */
/*             "1_8_straight",   # 17 */

/*             "9_10_straight",  # 18 */
/*             "10_9_straight",  # 19 */

/*             "11_10_straight", # 20 */
/*             "10_11_straight", # 21 */

/*             "11_3_straight",  # 22 */
/*             "3_11_straight",  # 23 */

/*             "10_13_straight", # 24 */
/*             "13_10_straight", # 25 */

/*             "12_13_straight", # 26 */
/*             "13_12_straight", # 27 */

/*             "12_8_straight",  # 28 */
/*             "8_12_straight",  # 29 */
/*         ] */

/*         for n in range(len(self.straight_nodes)): */
/*             self.G.add_node(self.straight_nodes[n]) */ 

   std::vector<std::string> turn_nodes {
      "2_8_right",   // 30 
      "8_2_left",    // 31 
                         
      "1_4_left",    // 32 
      "4_1_right",   // 33 
                         
      "3_4_right",   // 34 
      "4_3_left",    // 35 
                         
      "2_5_right",   // 36 
      "5_2_left",    // 37 
                         
      "4_7_left",    // 38 
      "7_4_right",   // 39 
                         
      "5_6_left",    // 40 
      "6_5_right",   // 41 
                         
      "7_9_right",   // 42 
      "9_7_left",    // 43 
                         
      "6_8_left",    // 44 
      "8_6_right",   // 45 
                         
      "6_10_right",  // 46 
      "10_6_left",   // 47 
                         
      "1_9_right",   // 48 
      "9_1_left",    // 49 
                         
      "9_12_right",  // 50 
      "12_9_left",   // 51 
                         
      "8_13_right",  // 52 
      "13_8_left",   // 53 
                         
      "12_10_right", // 54 
      "10_12_left",  // 55 
                         
      "9_13_left",   // 56 
      "13_9_right",  // 57 
                         
      "10_3_right",  // 58 
      "3_10_left",   // 59 
                         
      "11_13_right", // 60 
      "13_11_left",  // 61 
                         
      "11_2_right",  // 62 
      "2_11_left" }; // 63

/*     def add_turn_nodes(self): */
/*         #naming convention: start_finish_direction */
/*         self.turn_nodes = [ */
/*             "2_8_right",   # 30 */
/*             "8_2_left",    # 31 */

/*             "1_4_left",    # 32 */
/*             "4_1_right",   # 33 */

/*             "3_4_right",   # 34 */
/*             "4_3_left",    # 35 */

/*             "2_5_right",   # 36 */
/*             "5_2_left",    # 37 */

/*             "4_7_left",    # 38 */
/*             "7_4_right",   # 39 */

/*             "5_6_left",    # 40 */
/*             "6_5_right",   # 41 */

/*             "7_9_right",   # 42 */
/*             "9_7_left",    # 43 */

/*             "6_8_left",    # 44 */
/*             "8_6_right",   # 45 */

/*             "6_10_right",  # 46 */
/*             "10_6_left",   # 47 */

/*             "1_9_right",   # 48 */
/*             "9_1_left",    # 49 */

/*             "9_12_right",  # 50 */
/*             "12_9_left",   # 51 */

/*             "8_13_right",  # 52 */
/*             "13_8_left",   # 53 */

/*             "12_10_right", # 54 */
/*             "10_12_left",  # 55 */

/*             "9_13_left",   # 56 */
/*             "13_9_right",  # 57 */

/*             "10_3_right",  # 58 */
/*             "3_10_left",   # 59 */

/*             "11_13_right", # 60 */
/*             "13_11_left",  # 61 */

/*             "11_2_right",  # 62 */
/*             "2_11_left"    # 63 */
/*         ] */

   for (int i=0; i<straight_nodes.size(); i++) {
      Node n=main_map.addNode();
      name[n] = straight_nodes[i];
      name2node.emplace(straight_nodes[i], n);
   }

   for (int i=0; i<turn_nodes.size(); i++) {
      Node n=main_map.addNode();
      name[n] = turn_nodes[i];
      name2node.emplace(turn_nodes[i], n);
   }

   std::cout << "Nodes:";
   for (NodeIt i(main_map); i!=lemon::INVALID; ++i)
      std::cout << " " << main_map.id(i);
   std::cout << std::endl;

   std::cout << "Names:";
   for (NodeIt i(main_map); i!=lemon::INVALID; ++i) {
      std::cout << " " << name[i] << " " << main_map.id(i) << std::endl;
   }
   std::cout << std::endl;

   /////////////////////////add the edges
   vector<pair<std::string,std::string>> edge_names;

   /* def add_edges(self): */
   edge_names.push_back(std::make_pair("4_2_straight", "4_3_left"));      
   edge_names.push_back(std::make_pair("4_2_straight", "4_1_right"));     

   edge_names.push_back(std::make_pair("4_3_left", "2_3_straight"));      
   edge_names.push_back(std::make_pair("4_1_right", "2_1_straight"));     

   edge_names.push_back(std::make_pair("2_3_straight", "2_11_left"));     
   edge_names.push_back(std::make_pair("2_1_straight", "2_8_right"));     

   edge_names.push_back(std::make_pair("2_11_left", "3_11_straight"));    
   edge_names.push_back(std::make_pair("2_8_right", "1_8_straight"));     

   edge_names.push_back(std::make_pair("3_11_straight", "3_10_left"));    
   edge_names.push_back(std::make_pair("1_8_straight", "1_9_right"));     

   edge_names.push_back(std::make_pair("3_10_left", "11_10_straight"));   
   edge_names.push_back(std::make_pair("11_10_straight", "11_13_right")); 
   edge_names.push_back(std::make_pair("11_13_right", "10_13_straight")); 

   edge_names.push_back(std::make_pair("10_11_straight", "10_3_right"));  
   edge_names.push_back(std::make_pair("10_3_right", "11_3_straight"));   
   edge_names.push_back(std::make_pair("11_3_straight", "11_2_right"));   
   edge_names.push_back(std::make_pair("11_2_right", "3_2_straight"));    
   edge_names.push_back(std::make_pair("3_2_straight", "3_4_right"));     
   edge_names.push_back(std::make_pair("3_4_right", "2_4_straight"));     

   edge_names.push_back(std::make_pair("2_4_straight", "2_5_right"));     
   edge_names.push_back(std::make_pair("2_5_right", "4_5_straight"));     
   edge_names.push_back(std::make_pair("4_5_straight", "4_7_left"));      
   edge_names.push_back(std::make_pair("4_7_left", "5_7_straight"));      
   edge_names.push_back(std::make_pair("5_7_straight", "5_6_left"));      
   edge_names.push_back(std::make_pair("5_6_left", "7_6_straight"));      
   edge_names.push_back(std::make_pair("7_6_straight", "7_9_right"));     
   edge_names.push_back(std::make_pair("7_9_right", "6_9_straight"));     

   edge_names.push_back(std::make_pair("6_9_straight", "6_10_right"));    
   edge_names.push_back(std::make_pair("6_9_straight", "6_8_left"));      

   edge_names.push_back(std::make_pair("6_10_right", "9_10_straight"));   
   edge_names.push_back(std::make_pair("6_8_left", "9_8_straight"));      

   edge_names.push_back(std::make_pair("9_10_straight", "9_13_left"));    

   edge_names.push_back(std::make_pair("9_8_straight", "9_12_right"));    
   edge_names.push_back(std::make_pair("9_8_straight", "9_1_left"));      

   edge_names.push_back(std::make_pair("9_13_left", "10_13_straight"));   

   edge_names.push_back(std::make_pair("9_12_right", "8_12_straight"));   
   edge_names.push_back(std::make_pair("9_1_left", "8_1_straight"));      

   edge_names.push_back(std::make_pair("10_13_straight", "10_12_left"));  
   edge_names.push_back(std::make_pair("8_12_straight", "8_13_right"));   
   edge_names.push_back(std::make_pair("8_1_straight", "8_2_left"));      

   edge_names.push_back(std::make_pair("10_12_left", "13_12_straight"));  
   edge_names.push_back(std::make_pair("8_13_right", "12_13_straight"));  
   edge_names.push_back(std::make_pair("8_2_left", "1_2_straight"));      

   edge_names.push_back(std::make_pair("13_12_straight", "13_8_left"));   
   edge_names.push_back(std::make_pair("12_13_straight", "12_10_right")); 
   edge_names.push_back(std::make_pair("1_2_straight", "1_4_left"));      

   edge_names.push_back(std::make_pair("13_8_left", "12_8_straight"));    
   edge_names.push_back(std::make_pair("12_10_right", "13_10_straight")); 

   edge_names.push_back(std::make_pair("12_8_straight", "12_9_left"));    
   edge_names.push_back(std::make_pair("13_10_straight", "13_9_right"));  
   edge_names.push_back(std::make_pair("13_10_straight", "13_11_left"));  

   edge_names.push_back(std::make_pair("12_9_left", "8_9_straight"));     
   edge_names.push_back(std::make_pair("13_9_right", "10_9_straight"));   

   edge_names.push_back(std::make_pair("8_9_straight", "8_6_right"));     
   edge_names.push_back(std::make_pair("10_9_straight", "10_6_left"));    

   edge_names.push_back(std::make_pair("8_6_right", "9_6_straight"));     
   edge_names.push_back(std::make_pair("10_6_left", "9_6_straight"));     

   edge_names.push_back(std::make_pair("9_6_straight", "9_7_left"));      
   edge_names.push_back(std::make_pair("9_7_left", "6_7_straight"));      
   edge_names.push_back(std::make_pair("6_7_straight", "6_5_right"));     
   edge_names.push_back(std::make_pair("6_5_right", "7_5_straight"));     
   edge_names.push_back(std::make_pair("7_5_straight", "7_4_right"));     
   edge_names.push_back(std::make_pair("7_4_right", "5_4_straight"));     
   edge_names.push_back(std::make_pair("5_4_straight", "5_2_left"));      
   edge_names.push_back(std::make_pair("5_2_left", "4_2_straight"));      

   edge_names.push_back(std::make_pair("1_9_right", "8_9_straight"));     
   edge_names.push_back(std::make_pair("1_4_left", "2_4_straight"));      
   edge_names.push_back(std::make_pair("13_11_left", "10_11_straight"));  

   for (int i=0; i<edge_names.size(); i++) {
      /* Edge e=main_map.addEdge(); */
      auto temp_edge = edge_names[i];
      Node first_node = name2node[temp_edge.first];
      Node second_node = name2node[temp_edge.second];
      /* cout << first_node; */
      Edge e=main_map.addEdge(first_node, second_node);
      /* name2node.emplace(straight_nodes[i], n); */
   }

   std::cout << "Edges:";
   for (EdgeIt i(main_map); i!=lemon::INVALID; ++i) {
      std::cout << " (" << main_map.id(main_map.u(i)) << "," << main_map.id(main_map.v(i)) << ")";
   }
   /* std::cout << std::endl; */
   /* std::cout <<  std::endl; */

}

