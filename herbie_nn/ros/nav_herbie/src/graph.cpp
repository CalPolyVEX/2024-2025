//This file contains functions for the graph map

#include "navigation.h"

/* #include <lemon/smart_graph.h> */
/* #include <lemon/dijkstra.h> */

extern int sim_mode;

using namespace std;

/* lemon::SmartDigraph main_map; */
/* lemon::SmartDigraph::NodeMap<std::string> nodeMap(main_map); */
/* lemon::SmartDigraph::ArcMap<double> costMap(main_map); */ 

void Navigation::odom_callback(const nav_msgs::Odometry::ConstPtr& msg) { 
   float w = msg->pose.pose.orientation.w; 
   float x = msg->pose.pose.orientation.x; 
   float y = msg->pose.pose.orientation.y; 
   float z = msg->pose.pose.orientation.z; 

   heading_counter++;

   if (sim_mode == 1) { //the T265 publishes odometry at 200Hz, so slow it down
      if ((heading_counter & 3) == 0) { //compute the heading every 32 odometry messages
         heading_counter = 0;
         convert_to_heading(w, x, y, z); 
      }
   } else { //if not in sim mode, then odometry is published at 30Hz
      convert_to_heading(w, x, y, z); 
   }
}

void Navigation::convert_to_heading(float w, float x, float y, float z) {
   float t3, t4, heading;

   // convert the quaternion to a heading
   /* w = msg.pose.pose.orientation.w; */
   /* x = msg.pose.pose.orientation.x; */
   /* y = msg.pose.pose.orientation.y; */
   /* z = msg.pose.pose.orientation.z; */
   t3 = +2.0 * (w * z + x * y);
   t4 = +1.0 - 2.0 * (y * y + z * z);
   heading = 57.2958 * atan2(t3, t4);

   // this code handles the wrap-around of the heading from +180 to -180
   if (last_heading > 175 && heading < -175) {
      heading_offset += 360.0;
   } else if (last_heading < -175 && heading > 175) {
      heading_offset -= 360.0;
   }

   actual_heading = heading + heading_offset;
            
   last_heading = heading;  // move to next
}

void Navigation::graph_init() {
   /* std::map<std::string, int> nodes1 = { */
   /*    //straight nodes */
   /*    std::make_pair("1_2_straight",   0),   //0 */   
   /*    std::make_pair("2_1_straight",   1),   //1 */ 
               
   /*    std::make_pair("2_3_straight",   2),   //2 */ 
   /*    std::make_pair("3_2_straight",   3),   //3 */ 
              
   /*    std::make_pair("2_4_straight",   4),   //4 */ 
   /*    std::make_pair("4_2_straight",   5),   //5 */ 
             
   /*    std::make_pair("5_4_straight",   6),   //6 */ 
   /*    std::make_pair("4_5_straight",   7),   //7 */ 
            
   /*    std::make_pair("5_7_straight",   8),   //8 */ 
   /*    std::make_pair("7_5_straight",   9),   //9 */ 
           
   /*    std::make_pair("6_7_straight",   10),  //10 */ 
   /*    std::make_pair("7_6_straight",   11),  //11 */ 
          
   /*    std::make_pair("6_9_straight",   12),  //12 */ 
   /*    std::make_pair("9_6_straight",   13),  //13 */ 
              
   /*    std::make_pair("8_9_straight",   14),  //14 */ 
   /*    std::make_pair("9_8_straight",   15),  //15 */ 
             
   /*    std::make_pair("8_1_straight",   16),  //16 */ 
   /*    std::make_pair("1_8_straight",   17),  //17 */ 

   /*    std::make_pair("9_10_straight",  18),  //18 */ 
   /*    std::make_pair("10_9_straight",  19),  //19 */ 
            
   /*    std::make_pair("11_10_straight", 20),  //20 */
   /*    std::make_pair("10_11_straight", 21),  //21 */
           
   /*    std::make_pair("11_3_straight",  22),  //22 */
   /*    std::make_pair("3_11_straight",  23),  //23 */
          
   /*    std::make_pair("10_13_straight", 24),  //24 */
   /*    std::make_pair("13_10_straight", 25),  //25 */
         
   /*    std::make_pair("12_13_straight", 26),  //26 */
   /*    std::make_pair("13_12_straight", 27),  //27 */
        
   /*    std::make_pair("12_8_straight",  28),  //28 */
   /*    std::make_pair("8_12_straight",  29),  //29 */

   /*    //turn nodes */

   /*    std::make_pair("2_8_right",      30),  // 30 */ 
   /*    std::make_pair("8_2_left",       31),  // 31 */ 

   /*    std::make_pair("1_4_left",       32),  // 32 */ 
   /*    std::make_pair("4_1_right",      33),  // 33 */ 

   /*    std::make_pair("3_4_right",      34),  // 34 */ 
   /*    std::make_pair("4_3_left",       35),  // 35 */ 
      
   /*    std::make_pair("2_5_right",      36),  // 36 */ 
   /*    std::make_pair("5_2_left",       37),  // 37 */ 
     
   /*    std::make_pair("4_7_left",       38),  // 38 */ 
   /*    std::make_pair("7_4_right",      39),  // 39 */ 
    
   /*    std::make_pair("5_6_left",       40),  // 40 */ 
   /*    std::make_pair("6_5_right",      41),  // 41 */ 
   
   /*    std::make_pair("7_9_right",      42),  // 42 */ 
   /*    std::make_pair("9_7_left",       43),  // 43 */ 
  
   /*    std::make_pair("6_8_left",       44),  // 44 */ 
   /*    std::make_pair("8_6_right",      45),  // 45 */ 
 
   /*    std::make_pair("6_10_right",     46),  // 46 */ 
   /*    std::make_pair("10_6_left",      47),  // 47 */ 

   /*    std::make_pair("1_9_right",      48),  // 48 */ 
   /*    std::make_pair("9_1_left",       49),  // 49 */ 

   /*    std::make_pair("9_12_right",     50),  // 50 */ 
   /*    std::make_pair("12_9_left",      51),  // 51 */ 

   /*    std::make_pair("8_13_right",     52),  // 52 */ 
   /*    std::make_pair("13_8_left",      53),  // 53 */ 

   /*    std::make_pair("12_10_right",    54),  // 54 */ 
   /*    std::make_pair("10_12_left",     55),  // 55 */ 

   /*    std::make_pair("9_13_left",      56),  // 56 */ 
   /*    std::make_pair("13_9_right",     57),  // 57 */ 

   /*    std::make_pair("10_3_right",     58),  // 58 */ 
   /*    std::make_pair("3_10_left",      59),  // 59 */ 

   /*    std::make_pair("11_13_right",    60),  // 60 */ 
   /*    std::make_pair("13_11_left",     61),  // 61 */ 

   /*    std::make_pair("11_2_right",     62),  // 62 */ 
   /*    std::make_pair("2_11_left",      63)}; // 63 */

   /*    //straight nodes */
   /*    nodes.push_back("1_2_straight");   //0 */   
   /*    nodes.push_back("2_1_straight");   //1 */ 

   /*    nodes.push_back("2_3_straight");   //2 */ 
   /*    nodes.push_back("3_2_straight");   //3 */ 
         
   /*    nodes.push_back("2_4_straight");   //4 */ 
   /*    nodes.push_back("4_2_straight");   //5 */ 
      
   /*    nodes.push_back("5_4_straight");   //6 */ 
   /*    nodes.push_back("4_5_straight");   //7 */ 
     
   /*    nodes.push_back("5_7_straight");   //8 */ 
   /*    nodes.push_back("7_5_straight");   //9 */ 
    
   /*    nodes.push_back("6_7_straight");   //10 */ 
   /*    nodes.push_back("7_6_straight");   //11 */ 
   
   /*    nodes.push_back("6_9_straight");   //12 */ 
   /*    nodes.push_back("9_6_straight");   //13 */ 
  
   /*    nodes.push_back("8_9_straight");   //14 */ 
   /*    nodes.push_back("9_8_straight");   //15 */ 
 
   /*    nodes.push_back("8_1_straight");   //16 */ 
   /*    nodes.push_back("1_8_straight");   //17 */ 

   /*    nodes.push_back("9_10_straight");  //18 */ 
   /*    nodes.push_back("10_9_straight");  //19 */ 

   /*    nodes.push_back("11_10_straight"); //20 */
   /*    nodes.push_back("10_11_straight"); //21 */

   /*    nodes.push_back("11_3_straight");  //22 */
   /*    nodes.push_back("3_11_straight");  //23 */

   /*    nodes.push_back("10_13_straight"); //24 */
   /*    nodes.push_back("13_10_straight"); //25 */

   /*    nodes.push_back("12_13_straight"); //26 */
   /*    nodes.push_back("13_12_straight"); //27 */

   /*    nodes.push_back("12_8_straight");  //28 */
   /*    nodes.push_back("8_12_straight");  //29 */

   /*    //turn nodes */

   /*    nodes.push_back("2_8_right");      // 30 */ 
   /*    nodes.push_back("8_2_left");       // 31 */ 

   /*    nodes.push_back("1_4_left");       // 32 */ 
   /*    nodes.push_back("4_1_right");      // 33 */ 

   /*    nodes.push_back("3_4_right");      // 34 */ 
   /*    nodes.push_back("4_3_left");       // 35 */ 

   /*    nodes.push_back("2_5_right");      // 36 */ 
   /*    nodes.push_back("5_2_left");       // 37 */ 
     
   /*    nodes.push_back("4_7_left");       // 38 */ 
   /*    nodes.push_back("7_4_right");      // 39 */ 
    
   /*    nodes.push_back("5_6_left");       // 40 */ 
   /*    nodes.push_back("6_5_right");      // 41 */ 
   
   /*    nodes.push_back("7_9_right");      // 42 */ 
   /*    nodes.push_back("9_7_left");       // 43 */ 
  
   /*    nodes.push_back("6_8_left");       // 44 */ 
   /*    nodes.push_back("8_6_right");      // 45 */ 
 
   /*    nodes.push_back("6_10_right");     // 46 */ 
   /*    nodes.push_back("10_6_left");      // 47 */ 

   /*    nodes.push_back("1_9_right");      // 48 */ 
   /*    nodes.push_back("9_1_left");       // 49 */ 

   /*    nodes.push_back("9_12_right");     // 50 */ 
   /*    nodes.push_back("12_9_left");      // 51 */ 

   /*    nodes.push_back("8_13_right");     // 52 */ 
   /*    nodes.push_back("13_8_left");      // 53 */ 

   /*    nodes.push_back("12_10_right");    // 54 */ 
   /*    nodes.push_back("10_12_left");     // 55 */ 

   /*    nodes.push_back("9_13_left");      // 56 */ 
   /*    nodes.push_back("13_9_right");     // 57 */ 

   /*    nodes.push_back("10_3_right");     // 58 */ 
   /*    nodes.push_back("3_10_left");      // 59 */ 

   /*    nodes.push_back("11_13_right");    // 60 */ 
   /*    nodes.push_back("13_11_left");     // 61 */ 

   /*    nodes.push_back("11_2_right");     // 62 */ 
   /*    nodes.push_back("2_11_left");      // 63 */

   /* //define the edges */
   /* std::vector<Arc> arcs = { */
   /*    Arc {"4_2_straight", "4_3_left",        1},     //0 */ 
   /*    Arc {"4_2_straight", "4_1_right",       1},     //1 */

   /*    Arc {"4_3_left", "2_3_straight",        1},     //2 */
   /*    Arc {"4_1_right", "2_1_straight",       1},     //3 */

   /*    Arc {"2_3_straight", "2_11_left",       1},     //4 */
   /*    Arc {"2_1_straight", "2_8_right",       1},     //5 */ 

   /*    Arc {"2_11_left", "3_11_straight",      1},     //6 */
   /*    Arc {"2_8_right", "1_8_straight",       1},     //7 */ 

   /*    Arc {"3_11_straight", "3_10_left",      1},     //8 */
   /*    Arc {"1_8_straight", "1_9_right",       1},     //9 */ 

   /*    Arc {"3_10_left", "11_10_straight",     1},     //10 */
   /*    Arc {"11_10_straight", "11_13_right",   1},     //11 */
   /*    Arc {"11_13_right", "10_13_straight",   1},     //12 */

   /*    Arc {"10_11_straight", "10_3_right",    1},     //13 */
   /*    Arc {"10_3_right", "11_3_straight",     1},     //14 */ 
   /*    Arc {"11_3_straight", "11_2_right",     1},     //15 */ 
   /*    Arc {"11_2_right", "3_2_straight",      1},     //16 */ 
   /*    Arc {"3_2_straight", "3_4_right",       1},     //17 */  
   /*    Arc {"3_4_right", "2_4_straight",       1},     //18 */  

   /*    Arc {"2_4_straight", "2_5_right",       1},     //19 */
   /*    Arc {"2_5_right", "4_5_straight",       1},     //20 */
   /*    Arc {"4_5_straight", "4_7_left",        1},     //21 */ 
   /*    Arc {"4_7_left", "5_7_straight",        1},     //22 */ 
   /*    Arc {"5_7_straight", "5_6_left",        1},     //23 */ 
   /*    Arc {"5_6_left", "7_6_straight",        1},     //24 */ 
   /*    Arc {"7_6_straight", "7_9_right",       1},     //25 */
   /*    Arc {"7_9_right", "6_9_straight",       1},     //26 */

   /*    Arc {"6_9_straight", "6_10_right",      1},     //27 */
   /*    Arc {"6_9_straight", "6_8_left",        1},     //28 */  

   /*    Arc {"6_10_right", "9_10_straight",     1},     //29 */
   /*    Arc {"6_8_left", "9_8_straight",        1},     //30 */   

   /*    Arc {"9_10_straight", "9_13_left",      1},     //31 */
                                                     
   /*    Arc {"9_8_straight", "9_12_right",      1},     //32 */
   /*    Arc {"9_8_straight", "9_1_left",        1},     //33 */

   /*    Arc {"9_13_left", "10_13_straight",     1},     //34 */

   /*    Arc {"9_12_right", "8_12_straight",     1},     //35 */
   /*    Arc {"9_1_left", "8_1_straight",        1},     //36 */   

   /*    Arc {"10_13_straight", "10_12_left",    1},     //37 */
   /*    Arc {"8_12_straight", "8_13_right",     1},     //38 */ 
   /*    Arc {"8_1_straight", "8_2_left",        1},     //39 */ 

   /*    Arc {"10_12_left", "13_12_straight",    1},     //40 */
   /*    Arc {"8_13_right", "12_13_straight",    1},     //41 */
   /*    Arc {"8_2_left", "1_2_straight",        1},     //42 */ 

   /*    Arc {"13_12_straight", "13_8_left",     1},     //43 */
   /*    Arc {"12_13_straight", "12_10_right",   1},     //44 */
   /*    Arc {"1_2_straight", "1_4_left",        1},     //45 */   

   /*    Arc {"13_8_left", "12_8_straight",      1},     //46 */
   /*    Arc {"12_10_right", "13_10_straight",   1},     //47 */

   /*    Arc {"12_8_straight", "12_9_left",      1},     //48 */
   /*    Arc {"13_10_straight", "13_9_right",    1},     //49 */
   /*    Arc {"13_10_straight", "13_11_left",    1},     //50 */ 

   /*    Arc {"12_9_left", "8_9_straight",       1},     //51 */
   /*    Arc {"13_9_right", "10_9_straight",     1},     //52 */

   /*    Arc {"8_9_straight", "8_6_right",       1},     //53 */
   /*    Arc {"10_9_straight", "10_6_left",      1},     //54 */

   /*    Arc {"8_6_right", "9_6_straight",       1},     //55 */
   /*    Arc {"10_6_left", "9_6_straight",       1},     //56 */

   /*    Arc {"9_6_straight", "9_7_left",        1},     //57 */
   /*    Arc {"9_7_left", "6_7_straight",        1},     //58 */
   /*    Arc {"6_7_straight", "6_5_right",       1},     //59 */
   /*    Arc {"6_5_right", "7_5_straight",       1},     //60 */
   /*    Arc {"7_5_straight", "7_4_right",       1},     //61 */
   /*    Arc {"7_4_right", "5_4_straight",       1},     //62 */
   /*    Arc {"5_4_straight", "5_2_left",        1},     //63 */
   /*    Arc {"5_2_left", "4_2_straight",        1},     //64 */

   /*    Arc {"1_9_right", "8_9_straight",       1},     //65 */
   /*    Arc {"1_4_left", "2_4_straight",        1},     //66 */ 
   /*    Arc {"13_11_left", "10_11_straight",    1},     //67 */
   
   /*    //the next edges connect straight paths through intersections */
   /*    Arc {"1_8_straight", "8_12_straight",   1},     //68 */
   /*    Arc {"12_8_straight", "8_1_straight",   1},     //69 */
   /*    Arc {"1_2_straight", "2_3_straight",    1},     //70 */
   /*    Arc {"3_2_straight", "2_1_straight",    1},     //71 */
   
   /*    Arc {"8_9_straight", "9_10_straight",   1},     //72 */
   /*    Arc {"9_10_straight", "10_11_straight", 1},     //73 */
   /*    Arc {"11_10_straight", "10_9_straight", 1},     //74 */
   /*    Arc {"10_9_straight", "9_8_straight",   1}      //75 */
   /* }; */    

   /* //populate graph */
   /* //nodes first */
   /* lemon::SmartDigraph::Node currentNode; */
   /* for (auto nodesIter = nodes.begin(); nodesIter != nodes.end(); ++nodesIter) */
   /* { */
   /*    string key = *nodesIter; */
   /*    /1* std::cout << key << std::endl; *1/ */
   /*    currentNode = main_map.addNode(); */
   /*    nodeMap[currentNode] = key; */
   /* } */

   /* //then the arcs with the costs through the cost map */
   /* lemon::SmartDigraph::Arc currentArc; */
   /* for (auto arcsIter = arcs.begin(); arcsIter != arcs.end(); ++arcsIter) */
   /* { */
   /*    int sourceIndex = getIndex(nodes,arcsIter->sourceID); */
   /*    int targetIndex = getIndex(nodes,arcsIter->targetID); */

   /*    lemon::SmartDigraph::Node sourceNode = main_map.nodeFromId(sourceIndex); */
   /*    lemon::SmartDigraph::Node targetNode = main_map.nodeFromId(targetIndex); */

   /*    currentArc = main_map.addArc(sourceNode, targetNode); */
   /*    costMap[currentArc] = arcsIter->cost; */
   /* } */


   /* path_to_next_goal(); */
   igraph_vector_t v1, v2;
   igraph_strvector_t vector_names;

   /* igraph_create(&gr, &v1, 0, 0); //create a graph with 5 edges */

   /* turn on attribute handling */
   igraph_set_attribute_table(&igraph_cattribute_table);

   igraph_empty(&gr, 64, 1); //create a directed graph with 0 vertices

   igraph_add_edge(&gr, 0, 2);    SETEAS(&gr, "turn_dir", 0, "S"); 
   igraph_add_edge(&gr, 0, 4);    SETEAS(&gr, "turn_dir", 1, "L"); 

   igraph_add_edge(&gr, 1, 17);   SETEAS(&gr, "turn_dir", 2, "R");

   igraph_add_edge(&gr, 2, 23);   SETEAS(&gr, "turn_dir", 3, "L");

   igraph_add_edge(&gr, 3, 1);    SETEAS(&gr, "turn_dir", 4, "S");
   igraph_add_edge(&gr, 3, 4);    SETEAS(&gr, "turn_dir", 5, "R");

   igraph_add_edge(&gr, 4, 7);    SETEAS(&gr, "turn_dir", 6, "R");

   igraph_add_edge(&gr, 5, 1);    SETEAS(&gr, "turn_dir", 7, "R");
   igraph_add_edge(&gr, 5, 2);    SETEAS(&gr, "turn_dir", 8, "L");

   igraph_add_edge(&gr, 6, 5);    SETEAS(&gr, "turn_dir", 9, "L");

   igraph_add_edge(&gr, 7, 8);    SETEAS(&gr, "turn_dir", 10, "L");
   igraph_add_edge(&gr, 8, 11);   SETEAS(&gr, "turn_dir", 11, "L");
   igraph_add_edge(&gr, 9, 6);    SETEAS(&gr, "turn_dir", 12, "R");
   igraph_add_edge(&gr, 10, 9);   SETEAS(&gr, "turn_dir", 13, "R");
   igraph_add_edge(&gr, 11, 12);  SETEAS(&gr, "turn_dir", 14, "R");

   igraph_add_edge(&gr, 12, 15);  SETEAS(&gr, "turn_dir", 15, "L");
   igraph_add_edge(&gr, 12, 18);  SETEAS(&gr, "turn_dir", 16, "R");

   igraph_add_edge(&gr, 13, 10);  SETEAS(&gr, "turn_dir", 17, "L");

   igraph_add_edge(&gr, 14, 13);  SETEAS(&gr, "turn_dir", 18, "R");
   igraph_add_edge(&gr, 14, 18);  SETEAS(&gr, "turn_dir", 19, "S");

   igraph_add_edge(&gr, 15, 16);  SETEAS(&gr, "turn_dir", 20, "L");
   igraph_add_edge(&gr, 15, 29);  SETEAS(&gr, "turn_dir", 21, "R");

   igraph_add_edge(&gr, 16, 0);   SETEAS(&gr, "turn_dir", 22, "L");

   igraph_add_edge(&gr, 17, 14);  SETEAS(&gr, "turn_dir", 23, "R");
   igraph_add_edge(&gr, 17, 29);  SETEAS(&gr, "turn_dir", 24, "S");

   igraph_add_edge(&gr, 18, 21);  SETEAS(&gr, "turn_dir", 25, "S");
   igraph_add_edge(&gr, 18, 24);  SETEAS(&gr, "turn_dir", 26, "L");

   igraph_add_edge(&gr, 19, 13);  SETEAS(&gr, "turn_dir", 27, "L");
   igraph_add_edge(&gr, 19, 15);  SETEAS(&gr, "turn_dir", 28, "S");

   igraph_add_edge(&gr, 20, 19);  SETEAS(&gr, "turn_dir", 29, "S");
   igraph_add_edge(&gr, 20, 24);  SETEAS(&gr, "turn_dir", 30, "R");

   igraph_add_edge(&gr, 21, 22);  SETEAS(&gr, "turn_dir", 31, "R");
   igraph_add_edge(&gr, 22, 3);   SETEAS(&gr, "turn_dir", 32, "R");
   igraph_add_edge(&gr, 23, 20);  SETEAS(&gr, "turn_dir", 33, "L");
   igraph_add_edge(&gr, 24, 27);  SETEAS(&gr, "turn_dir", 34, "L");

   igraph_add_edge(&gr, 25, 19);  SETEAS(&gr, "turn_dir", 35, "R");
   igraph_add_edge(&gr, 25, 21);  SETEAS(&gr, "turn_dir", 36, "L");

   igraph_add_edge(&gr, 26, 25);  SETEAS(&gr, "turn_dir", 37, "R");
   igraph_add_edge(&gr, 27, 28);  SETEAS(&gr, "turn_dir", 38, "L");

   igraph_add_edge(&gr, 28, 14);  SETEAS(&gr, "turn_dir", 39, "L");
   igraph_add_edge(&gr, 28, 16);  SETEAS(&gr, "turn_dir", 40, "S");

   igraph_add_edge(&gr, 29, 26);  SETEAS(&gr, "turn_dir", 41, "R");

   igraph_strvector_init(&vector_names, 0);
   /* igraph_vector_destroy(&v1); */

   SETVAS(&gr, "id", 1, "test");
   SETEAN(&gr, "weight", 2, 100.0); //set edge 2 weight to 100.0

   /* igraph_vector_init(&v2, 0); */
   /* igraph_get_edgelist(&gr, &v2, 0); */
   /* igraph_vector_sort(&v1); */
   /* igraph_vector_sort(&v2); */

   int n_vert = (int) igraph_vcount(&gr); //get the number of vertices
   int n_edge = (int) igraph_ecount(&gr);

   std::cout << "Graph init complete.\n";
   std::cout << "Number of vertices: " << n_vert << std::endl;
   std::cout << "Number of edges: " << n_edge << std::endl;

   for (int i=0; i<n_edge; i++) {
      igraph_integer_t from, to;
      igraph_edge(&gr, i, &from, &to);
      std::cout << "from: " << (int) from << "  to: " << (int)to << std::endl;
   }

   //printing test attribute
   std::cout << "vertex id: " << VAS(&gr, "id", 2) << std::endl;

   path_to_next_goal();
}

void Navigation::path_to_next_goal() {
   igraph_vector_t v,e;

   igraph_vector_init(&v, 100);
   igraph_vector_init(&e, 100);

   igraph_get_shortest_path(&gr, &v, &e, 4, 27, IGRAPH_OUT);

   int i=0;
   while ((int)VECTOR(v)[i] != 27) {
      std::cout << (int) VECTOR(v)[i] << std::endl;
      i++;
   }

   igraph_vector_destroy(&v);
   igraph_vector_destroy(&e);
}
