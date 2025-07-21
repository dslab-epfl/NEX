#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>  // Include the nlohmann/json library

using json = nlohmann::json;

#define WRITE_REG_TYPE 0
#define READ_REG_TYPE 1
#define START_TYPE 2
#define FINISH_TYPE 3

int main() {
    std::ifstream infile("log.txt");  // Open the input text file
    if (!infile) {
        std::cerr << "Error opening file!" << std::endl;
        return 1;
    }

    json finalJson;
    json jsonArray = json::array();  // Create a JSON array
    std::string line;

    json jsonObject;
    finalJson["displayTimeUnit"] = "ns";

    while (std::getline(infile, line)) {  // Read the file line by line
        std::stringstream ss(line);
        std::string item;
        json jsonObject;  // Create a JSON object for each line

        int index = 1;
        bool is_cpu = 0;
        while (std::getline(ss, item, ',')) {  // Split the line by commas
            int number = std::stoi(item);  // Convert string to integer
            if(index == 1){
                is_cpu = 0;
                if(number == 0){
                    is_cpu = 1;
                }
                jsonObject["pid"] = number;
                jsonObject["tid"] = number;
            }else if(index == 2){
                jsonObject["ts"] = number/1000;
            }else if(index == 3){
                if(!is_cpu){
                    switch (number){
                        case WRITE_REG_TYPE:
                            jsonObject["name"] = "WRITE_REG";
                            jsonObject["ph"] = "i";
                            break;
                        case READ_REG_TYPE:
                            jsonObject["name"] = "READ_REG";
                            jsonObject["ph"] = "i";
                            break;
                        case START_TYPE:
                            jsonObject["name"] = "AccelSim";
                            jsonObject["ph"] = "B";
                            break;
                        case FINISH_TYPE:
                            // name not used
                            jsonObject["name"] = "FINISH";
                            jsonObject["ph"] = "E";
                            break;
                        default:
                            jsonObject["name"] = "UNKNOWN";
                            jsonObject["ph"] = "i";
                            break;
                    }
                }else{
                    jsonObject["name"] = std::to_string(number);  // Add the value to the JSON object
                    jsonObject["ph"] = "X";
                    jsonObject["dur"] = 1;
                }
            }
            ++index;
        }

        jsonArray.push_back(jsonObject);  // Add the JSON object to the JSON array
    }

    infile.close();  // Close the input file

    finalJson["traceEvents"] = jsonArray;
    // Write the JSON array to the output file
    std::ofstream outfile("output.json");  // Open the output JSON file
    outfile << finalJson.dump(4);  // Pretty print with 4 spaces of indentation
    outfile.close();  // Close the output file

    std::cout << "JSON array has been written to output.json" << std::endl;

    return 0;
}
