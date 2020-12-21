
#pragma once

// Data pertaining to a TIM which has been uploaded to VRAM
// so we can make sprites from it :)
typedef struct TIMData {

	unsigned long clutX;
    unsigned long clutY;

	unsigned long vramX;
    unsigned long vramY;

    unsigned long vramWidth;
    unsigned long vramHeight;

    unsigned long texPage; // saves calculating it per-frame

    // relative to the tex page, where applicable
    unsigned long pixU;
    unsigned long pixV;

} TIMData;



void UploadTim( 
    const char* inTim, TIMData * data, 
    unsigned long clutX, unsigned long clutY, unsigned 
    long pixX, unsigned long pixY  
);
