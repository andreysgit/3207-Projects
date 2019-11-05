//
// Created by Andrey on 11/5/19.
//

#ifndef NETWORKSPELLCHECK_SPELLCHECK_H
#define NETWORKSPELLCHECK_SPELLCHECK_H
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
std::ifstream mydictionaryFile;
std::string const testdefault_dictionary = "words.txt";
std::vector<std::string> testwords;
std::string testline="";

namespace sc{
    class spellcheck {
    public:
        explicit spellcheck(std::string word){
        //word_lookup searches string vector table for input string
        {
                mydictionaryFile.open(testdefault_dictionary);
            if(mydictionaryFile.fail()){
                std::cerr<<"Error opening file";}

            if(mydictionaryFile.is_open()) {//init vector
                    std::cout << "\nDictionary has been opened.";
                std::cout << "\nSearching for element: " << word;
                while (std::getline(mydictionaryFile, testline)) {
                        testwords.push_back(testline);
                    }
                }
                mydictionaryFile.close();


                if(std::find(testwords.begin(),testwords.end(),word)!=testwords.end()){
                    std::cout << "\nElement found";
                }
                else{
                    std::cout<< "\nno match";
                }
        }
        }
    };
}


#endif //NETWORKSPELLCHECK_SPELLCHECK_H
