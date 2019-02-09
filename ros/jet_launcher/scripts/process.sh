#This script combines multiple rtabmap databases into a large single database

rtabmap-reprocess "8am_output1.db;8am_output2.db;8am_output3.db;8_30am_output1.db;9am_output1.db;10am_output1.db;10am_output2.db;10am_output3.db;12pm_output1.db;2pm_output1.db" "output.db"
#rtabmap-reprocess --Mem/BinDataKept false "8am_output1.db;8am_output2.db;8am_output3.db;8_30am_output1.db;9am_output1.db;10am_output1.db;10am_output2.db;10am_output3.db;12pm_output1.db;2pm_output1.db" "output.db"

