#pragma once

#include "Colorizer.h"

class StudioColorizer :public Colorizer
{
public:
	StudioColorizer(Logger logger, const char* colorAttribute = nullptr);
	void init(Store& store) override;
};