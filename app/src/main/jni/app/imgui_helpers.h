bool block_add(const char *label, const ImVec2& size_arg, ImRect *pFrame_bb, bool *pHovered);
void get_max_min(float *pData, int values_count, float *scale_max, float *scale_min);
void draw_lines(ImRect frame_bb, float *pData, int values_count, ImU32 col, float scale_min_y = FLT_MAX, float scale_max_y = FLT_MAX);