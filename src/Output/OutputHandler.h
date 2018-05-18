#pragma once

#include "../Variant.h"
#include <iostream>  
#include <fstream>


inline void createFile(std::string outputDir){

	std::string pfile = outputDir + "/pvalues.txt";
	std::string ffile = outputDir + "/filtered.txt";

	std::ofstream pvals(pfile);	
	pvals.close();


	std::ofstream filtered(ffile);	
	filtered.close();
}

inline void outputPvals(std::vector<Variant> variants, std::string outputDir) {
	
	std::string pfile = outputDir + "/pvalues.txt";
	std::ofstream pvals(pfile, std::ios_base::app);

	if (pvals.is_open())
	{
        for (size_t i = 0; i < variants.size(); i++) {

            pvals << variants[i].toString() << '\t';
            pvals << variants[i].pvalue << '\n';
		}
		pvals.close();
	}
}

inline void outputPvals(std::vector<Variant> variants, std::string outputDir, int collapse) {

	//todo: output collapsed information

	std::string pfile = outputDir + "/pvalues.txt";
	std::ofstream pvals(pfile, std::ios_base::app);

	int counter=0;
	int pindex=0;
	if (pvals.is_open())
	{
        for (size_t i = 0; i < variants.size(); i++) {

			counter++;
			if(counter > collapse){
			  pindex++;
			  counter=0;			
			}

            pvals << variants[i].toString() << '\t';
            pvals << variants[pindex].pvalue << '\n';
		}
		pvals.close();
	}
}

inline void outputFiltered(std::vector<Variant> variants, std::string explain, std::string outputDir) {

	std::string ffile = outputDir + "/filtered.txt";
	std::ofstream filtered(ffile, std::ios_base::app);

	if (filtered.is_open())
	{
        for (size_t i = 0; i < variants.size(); i++) {

            filtered << variants[i].toString() << '\t';
			filtered << explain << '\n';
		}
		filtered.close();
	}
}

