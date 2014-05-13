/*------------------------------LQR controller -------------------------------*/
double e1_gain[max_number_of_apps][4] = {{-.0390,  0.6150, -.0832, 0.0260}, {-.2423,  0.5212, -.0803, 0.0205},{-.6990, -.3005, -.0917, -0.0232}};
double e2_gain[max_number_of_apps][4] = {{-.3985, -1.2376, -.0311, -.0949}, {-.3041, -1.1219, -.0263, -.0962}, {0.1143, -1.2857, .0211, -.0958}};
int operating_band[max_number_of_apps] = {65, 72, 65};
int operating_period[max_number_of_apps] = {90, 90, 90};
int alpha_min[max_number_of_apps] = {57, 59, 59};
int alpha_max[max_number_of_apps] = {73, 85, 80};
int T_min[max_number_of_apps] = {40, 65, 65};
int T_max[max_number_of_apps] = {140, 125, 125};
int r1[max_number_of_apps] = {2, 6, 6};
int r2[max_number_of_apps] = {1, 1, 1};
char log_file[max_number_of_apps][15] = {"lqr_log0.txt", "lqr_log1.txt", "lqr_log2.txt"};