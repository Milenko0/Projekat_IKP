#pragma once


typedef enum MeasurementTopic { Analog = 0, Status }Topic;
typedef enum MeasurementType { SWG = 0, CRB, MER }Type;

typedef struct measurementStruct {
    Topic topic;
    Type type;
    int value;

}Measurement;

