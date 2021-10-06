//This file contains functions for the graph map

#include "navigation.h"
#include <cstring>

extern int sim_mode;

using namespace std;

void Navigation::graph_init() {
   /* turn on attribute handling */
   igraph_set_attribute_table(&igraph_cattribute_table);

   igraph_empty(&gr, 64, 1); //create a directed graph with 64 vertices

   //In the graph, vertices are hallways and edges connect the hallways.
   //Each of the edges has a turn direction associated with the edge (Straight,
   //Left, or Right).
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

   for (int i=0; i<42; i++) { //set all hallway to not narrow
      SETVAN(&gr, "narrow", i, 0);
   }

   //set the narrow hallways
   SETVAN(&gr, "narrow", 4, 1);
   SETVAN(&gr, "narrow", 5, 1);
   SETVAN(&gr, "narrow", 12, 1);
   SETVAN(&gr, "narrow", 13, 1);
   SETVAN(&gr, "narrow", 22, 1);
   SETVAN(&gr, "narrow", 23, 1);

   int n_vert = (int) igraph_vcount(&gr); //get the number of vertices
   int n_edge = (int) igraph_ecount(&gr); //get the number of edges

   std::cout << "Graph initialization complete.\n";
   std::cout << "Number of vertices: " << n_vert << std::endl;
   std::cout << "Number of edges: " << n_edge << std::endl;

   //print all the edges
   /* for (int i=0; i<n_edge; i++) { */
   /*    igraph_integer_t from, to; */
   /*    igraph_edge(&gr, i, &from, &to); */
   /*    std::cout << "from: " << (int) from << "  to: " << (int)to << std::endl; */
   /* } */

   /* std::cout << get_next_turn_dir(1,18) << std::endl; */
}

void Navigation::path_to_next_goal() {
   int start = 17;
   int end = 22;

   igraph_vector_t vertices,edges;

   igraph_vector_init(&vertices, 20); //initialize to default size of 20
   igraph_vector_init(&edges, 20);

   igraph_get_shortest_path(&gr, &vertices, &edges, start, end, IGRAPH_OUT);

   //print out the vertices to get from start to end
   int i=0;
   int current_vertex = VECTOR(vertices)[0]; //initialize to the starting vertex

   while (current_vertex != end) {
      std::cout << current_vertex << std::endl;
      int last_vertex = current_vertex; 
      i++;
      current_vertex = VECTOR(vertices)[i];

      igraph_integer_t edge;
      int eid = igraph_get_eid(&gr, &edge, last_vertex, current_vertex, IGRAPH_DIRECTED, 0); //lookup the edge id
      if (eid != -1) {
         //printing turn_dir
         std::cout << "\tturn_dir: " << EAS(&gr, "turn_dir", edge) << std::endl;
      }
   }

   igraph_vector_destroy(&vertices);
   igraph_vector_destroy(&edges);
}

//returns the direction of the next turn in the path (0=left, 1=straight, 2=right)
int Navigation::get_next_turn_dir(int start, int end) {
   igraph_vector_t vertices,edges;

   igraph_vector_init(&vertices, 20); //initialize to default size of 20
   igraph_vector_init(&edges, 20);

   igraph_get_shortest_path(&gr, &vertices, &edges, start, end, IGRAPH_OUT);

   //print out the vertices to get from start to end
   int current_vertex = VECTOR(vertices)[0]; //initialize to the starting vertex
   int next_vertex = VECTOR(vertices)[1];    //initialize to the next vertex in path
   int direction = -1; //initialize to error
   
   igraph_integer_t edge;
   int eid = igraph_get_eid(&gr, &edge, current_vertex, next_vertex, IGRAPH_DIRECTED, 0); //lookup the edge id
   if (eid != -1) {
      const char* dir_string = EAS(&gr, "turn_dir", edge);

      if (strcmp("L", dir_string) == 0)
         direction = 0;
      else if (strcmp("S", dir_string) == 0)
         direction = 1;
      else if (strcmp("R", dir_string) == 0)
         direction = 2;
   }

   igraph_vector_destroy(&vertices);
   igraph_vector_destroy(&edges);

   return direction;
}

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

   heading_mutex.lock();
   actual_heading = heading + heading_offset;
   heading_mutex.unlock();
            
   last_heading = heading;  // move to next
}
