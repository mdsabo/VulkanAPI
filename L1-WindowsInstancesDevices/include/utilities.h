
#ifndef UTILITIES_H_
#define UTILITIES_H_

struct QueueFamilyIndicies
{
    int graphicsFamily{-1};

    bool isValid()
    {
        return graphicsFamily >= 0;
    }
};

#endif