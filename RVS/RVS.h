#pragma once
#include "stdafx.h"
#include "MemoryMapped/MemoryMapped.h"
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>  
#include "Eigen/Dense"
using Eigen::MatrixXd;
using Eigen::VectorXd;
using Eigen::VectorXi;
using Eigen::DiagonalMatrix;

//========================================================
// Group struct
//========================================================

struct Group {
	std::string ID = "";
	int groupIndex = 0;
	std::vector<size_t> index;
	bool hrg = false;
};

//========================================================
// Sample struct
//========================================================

struct Sample {
	std::string ID = "";
	int groupIndex = 0;
	std::string groupID = "";
	double y = 0;
	double hrg = 0;
	std::vector<double> covariates;

	void print() {
		std::cout << ID;
		std::cout << '\t';
		std::cout << y;
		std::cout << '\t';
		std::cout << groupID;
		std::cout << "->";
		std::cout << groupIndex;
		std::cout << '\t';
		std::cout << hrg;
		std::cout << '\t';
		for (size_t i = 0; i < covariates.size(); i++) {
			std::cout << covariates[i];
			std::cout << '\t';
		}
		std::cout << '\n';
	}
};

//========================================================
// SNP struct
//========================================================

struct genotypeLikelihood {
	bool valid = false;
	double L00;
	double L01;
	double L11;
};

struct SNP {
	std::string chr;
	int loc;

	std::vector<genotypeLikelihood> gl;
	std::vector<int> rd;

	std::vector<double> p;
	std::vector<double> EG;

	double maf = NAN;

	inline bool operator<(SNP& snp2) { return this->loc < snp2.loc; }
	inline double L00(int index) { return this->gl[index].L00; }
	inline double L01(int index) { return this->gl[index].L01; }
	inline double L11(int index) { return this->gl[index].L11; }

	SNP clone() {
		SNP newSNP;
		newSNP.chr = this->chr;
		newSNP.loc = this->loc;
		newSNP.gl = this->gl;
		newSNP.p = this->p;
		newSNP.EG = this->EG;
		newSNP.maf = this->maf;
		return newSNP;
	}

};

//========================================================
// Interval struct
//========================================================

struct Interval {
	int start = 0;
	int end = 0;
	std::vector<int> indexes;
	std::string chr;

	std::vector<SNP> getInInterval(std::vector<SNP> &snps) {
		std::vector<SNP> in;
		for (size_t i = 0; i < indexes.size(); i++)
			in.push_back(snps[indexes[i]]);

		return in;
	}

	bool addIfIn(SNP snp, int index) {

		if (snp.loc >= start && snp.loc <= end && chr.compare(snp.chr) == 0) {
			indexes.push_back(index);
			return true;
		}
		
		return false;
	}

};


inline bool locCompare(SNP lhs, SNP rhs) { return lhs < rhs; }


/**
Extracts string from a MemoryMapped class

@param charArray MemoryMapped to extract string from.
@param start Index to start extracting string from.
@param end Index to stop extracting string from.
@return Extracted string.
*/
inline std::string getString(MemoryMapped &charArray, int start, int end) {
	std::string ret;
	for (; start < end; start++) { ret += charArray[start]; }
	return ret;
}





//========================================================
// functions
//========================================================


//VCFParser.cpp
bool parseInput(std::string vcfDir, std::string infoDir, double mafCut, bool common,
	MatrixXd &X, VectorXd &Y, VectorXd &G, VectorXd &H, MatrixXd &Z);
std::vector<std::string> parseHeader(MemoryMapped &, int &);

//BEDParser.cpp
std::vector<Interval> getIntervals(std::string);
void collapseVariants(std::vector<SNP> &, std::vector<Interval> &);

//Statistics.cpp
double meanX(SNP &, Group &);
double meanY(std::vector<Sample> &, SNP &);
double varX(SNP &, Group &);
double var(VectorXd &);
double variance(std::vector<double> &);
double chiSquareOneDOF(double);
std::vector<double> randomSample(std::vector<double> &, int);
VectorXd CovariateRegression(VectorXd &Y, MatrixXd &Z);
int generateRandomInteger(int from, int to);
double generateRandomDouble (double from, double to);
double randomNormal(double mean, double sd);
MatrixXd nanToZero(MatrixXd &M);
VectorXd nanToZero(VectorXd &V);
MatrixXd covariance(MatrixXd &M);
MatrixXd correlation(MatrixXd &M);
double pnorm(double x);

//CommonTest.cpp
std::vector<double> runCommonTest(std::vector<SNP> &, std::vector<Sample> &, std::vector<Group> &, int nboot=0, bool rvs = true);

//RareTest.cpp
std::vector<double> runRareTest(std::vector<SNP> &snps, std::vector<Sample> &sample, std::vector<Group> &group, int nboot = 0, bool rvs = true);

//CompQuadForm.cpp
double qfc(std::vector<double>, double, int);

//Simulation.cpp
void simulate();

//========================================================
// inline functions
//========================================================


/*
Calculates the robust variance of E(G | D). var(x) = E(x^2) - E(x)^2

@param p Genotype frequency from EM algorithm
@return Robust variance of E(G | D)
*/
inline double calcRobustVar(std::vector<double> p) {
	return (4 * p[2] + p[1]) - pow(2 * p[2] + p[1], 2);
}


/*
Makes a copy of all SNPs

@param snps	SNPs to copy
@return vector of copied SNPs
*/
inline std::vector<SNP> cloneAll(std::vector<SNP> snps) {
	std::vector<SNP> clones;

	for (size_t i = 0; i < snps.size(); i++)
		clones.push_back(snps[i].clone());

	return clones;
}

//========================================================
// timing functions
//========================================================

#include <time.h>

inline clock_t startTime() {
	clock_t t;
	t = clock();
	return t;
}

inline void endTime(clock_t t, std::string label) {
	t = clock() - t;
	std::cout << "Timer " + label + " took ";
	std::cout << (float)t / CLOCKS_PER_SEC;
	std::cout << " seconds\n";
}

inline double endTime(clock_t t) {
	t = clock() - t;
	return (double)t / CLOCKS_PER_SEC;
}