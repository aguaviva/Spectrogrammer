void Init_waterfall(int16_t width, int16_t height);
void Draw_waterfall(ImRect frame_bb);
void Shutdown_waterfall();
void Draw_update(int16_t *pData, uint32_t size);
void Draw_update(float *pData, uint32_t size);

void Draw_hover_data(ImRect frame_bb, bool hovered);
void Draw_vertical_scale(ImRect frame_bb, float seconds_per_row);
