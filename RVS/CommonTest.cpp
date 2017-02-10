#include "stdafx.h"
#include "RVS.h"

#include <iostream>  
#include <math.h> 

/*
Calculates score test statistic.
score = sum((Y - Yhat) * X)
testStastic = score^2 / [sum((Y - Yhat)^2 * var(X))]

@param y Vector with phenotypes (case/control).
@param x Vector with E(G | D).
@return Test statistic following a chi-squared distribution with 1 degree of freedom.
*/
double scoreTest(std::vector<bool> &y, std::vector<double> &x) {

	double ybar = 0;
	double xbar = 0;

	double n = 0;

	for (size_t i = 0; i < y.size(); i++)
		if (x[i] != NULL) {
			ybar += y[i];
			xbar += x[i];
			n++;
		}

	ybar /= n;
	xbar /= n;
	double xvar = 0;

	for (size_t i = 0; i < x.size(); i++)
		if (x[i] != NULL)
			xvar += pow(x[i] - xbar, 2);

	xvar /= (n);
	double score = 0;
	double dnom = 0;
	double temp;

	for (size_t i = 0; i < y.size(); i++)
		if (x[i] != NULL) {
			temp = y[i] - ybar;

			score += temp * x[i];
			dnom += pow(temp, 2) * xvar;
		}
	return pow(score, 2) / dnom;
}

/*
Finds p-value for test statistic using a chi-squared distribution with one degree of freedom
using chi-squared probability density function.

@param statistic Test statistic.
@return p-value.
*/
double chiSquareOneDOF(double statistic) {
	double z = statistic * 0.5;
	double sc = 2 * sqrt(z) * exp(-z);

	double sum = 1;
	double prevSum = sum;
	double nom = 1;
	double dnom = 1;
	double s = 0.5;

	for (int i = 0; i < 200; i++)
	{
		nom *= z;
		s++;
		dnom *= s;
		sum += nom / dnom;
		if (prevSum == sum) break;
		prevSum = sum;
	}

	double p = sum * sc;
	if (isnan(p) || isinf(p) || p <= 1e-8)
		return 1e-14;

	p /= tgamma(0.5);

	return 1 - p;
}

/*
RVS using asymptotic distribution for score test statistic. This functions includes
two association tests which differ in the estimation of the variance of score. It uses
var_case(E(G | D)) when rvs = false and var_case(G) when rvs = true.

Reference: http://www.ncbi.nlm.nih.gov/pubmed/22570057
Reference: http://www.ncbi.nlm.nih.gov/pubmed/24733292
@param snps Vector of SNPs.
@param IDmap Vector with phenotypes (case/control).
@param rvs Indicates how to calculate variance
@return p-values for the SNPs
*/
std::vector<double> RVSasy(std::vector<SNP> &snps, std::vector<bool> &IDmap, bool rvs) {

	std::vector<double> pvals;
	SNP snp;
	double v;
	double p;

	for (size_t j = 0; j < snps.size(); j++) {
		snp = snps[j];

		if (rvs) {
			p = snp.ncase / snp.n;
			v = (1 - p) * calcRobustVar(snp.p) + p * snp.controlvar;
			v = snp.var / v;
		}
		else
			v = 1;

		pvals.push_back(chiSquareOneDOF(scoreTest(IDmap, snp.EG) * v));
	}

	return pvals;
}

/*
Bootstrap test called from RVSbtrap when rvs = true

@param snps Vector of SNPs.
@param IDmap Vector with phenotypes (case/control).
@param nboot Number of bootstrap iterations.
@param tobs Observed test statistic.
@return p-value from the bootstrap test.
*/
double bstrapHelp1(SNP &snp, std::vector<bool> &IDmap, int nboot, double tobs) {
	srand((int)time(NULL));

	std::vector<double> x0;
	std::vector<double> x1;
	std::vector<double> counter0;
	std::vector<double> counter1;

	for (size_t i = 0; i < snp.EG.size(); i++) {
		if (snp.EG[i] != NULL) {
			if (IDmap[i]) {
				x1.push_back(snp.EG[i] - snp.casemean);
				counter1.push_back(0);
			}
			else {
				x0.push_back(snp.EG[i] - snp.controlmean);
				counter0.push_back(0);
			}
		}
	}

	double bootmean0;
	double bootvar0;
	double bootmean1;
	double bootvar1;
	double statistic;
	double bootScoreCount = 1;
	int ncase = int(snp.ncase);
	int ncontrol = int(snp.ncontrol);

	for (int k = 0; k < nboot; k++) {
		bootmean0 = 0;
		bootvar0 = 0;
		bootmean1 = 0;
		bootvar1 = 0;
		//reset counters
		for (int i = 0; i < ncontrol; i++)
			counter0[i] = 0;
		for (int i = 0; i < ncase; i++)
			counter1[i] = 0;

		//sample from observed values
		for (int i = 0; i < ncontrol; i++)
			counter0[rand() % ncontrol]++;
		for (int i = 0; i < ncase; i++)
			counter1[rand() % ncase]++;

		//calculate means
		for (int i = 0; i < ncontrol; i++)
			bootmean0 += counter0[i] * x0[i];
		for (int i = 0; i < ncase; i++)
			bootmean1 += counter1[i] * x1[i];

		bootmean0 /= snp.ncontrol;
		bootmean1 /= snp.ncase;

		//calculate variances
		for (int i = 0; i < ncontrol; i++)
			bootvar0 += counter0[i] * pow(x0[i] - bootmean0, 2);
		for (int i = 0; i < ncase; i++)
			bootvar1 += counter1[i] * pow(x1[i] - bootmean1, 2);

		bootvar0 /= snp.ncontrol - 1;
		bootvar1 /= snp.ncase - 1;

		statistic = (bootmean1 - bootmean0) / sqrt(bootvar1 / snp.ncase + bootvar0 / snp.ncontrol);
		if (abs(statistic) >= tobs)
			bootScoreCount++;
	}

	return 	bootScoreCount / nboot;
}

/*
Bootstrap test called from RVSbtrap when rvs = false

@param snps Vector of SNPs.
@param IDmap Vector with phenotypes (case/control).
@param nboot Number of bootstrap iterations.
@param tobs Observed test statistic.
@return p-value from the bootstrap test.
*/
double bstrapHelp2(SNP &snp, std::vector<bool> &IDmap, int nboot, double tobs) {
	srand((int)time(NULL));

	std::vector<size_t> x;
	double bootScoreCount = 1;
	double p = snp.ncase / snp.n;
	double q = 1 - p;
	double vs = p * q * snp.n * snp.var;
	double statistic;
	double casesum;
	double controlsum;

	size_t xindex;

	for (size_t i = 0; i < snp.EG.size(); i++)
		if (snp.EG[i] != NULL)
			x.push_back(i);

	for (int k = 0; k < nboot; k++) {
		casesum = 0;
		controlsum = 0;
		xindex = 0;
		std::random_shuffle(x.begin(), x.end());

		for (size_t i = 0; i < snp.EG.size(); i++) {
			if (snp.EG[i] != NULL) {
				if (IDmap[i])
					casesum += snp.EG[x[xindex]];
				else
					controlsum += snp.EG[x[xindex]];
				xindex++;
			}
		}

		statistic = pow(q * casesum - p * controlsum, 2) / vs;

		if (abs(statistic) >= tobs)
			bootScoreCount++;
	}
	return 	bootScoreCount / nboot;
}

/*
Uses RVS to test associaton by bootstrap, given phenotype, expected values of genotypes, 
estimated genotype frequency and number of bootstrap iterations.

@param snps Vector of SNPs.
@param IDmap Vector with phenotypes (case/control).
@param nboot Number of bootstrap iterations.
@param rvs Indicates how to calculate variance of score statistic.
@return Vector of p-values for the SNPs.
*/
std::vector<double> RVSbtrap(std::vector<SNP> &snps, std::vector<bool> &IDmap, int nboot, bool rvs) {
	std::vector<double> pvals;
	SNP snp;

	double maf = 0;
	double p;
	double s;
	double tobs;

	for (size_t j = 0; j < snps.size(); j++) {
		snp = snps[j];

		//calculate observed score statistic
		if (rvs)
			tobs = (snp.casemean - snp.controlmean) /
			sqrt(calcRobustVar(snp.p) / snp.ncase + snp.controlvar / snp.ncontrol);
		else {
			p = snp.ncase / snp.n;
			s = (1 - p) * snp.casemean * snp.ncase - p * snp.controlmean * snp.ncontrol;
			tobs = pow(s, 2) / (p * (1 - p) * snp.n * snp.var);
		}
		tobs = abs(tobs);

		//bootstrap test
		if (rvs)
			pvals.push_back(bstrapHelp1(snp, IDmap, nboot, tobs));
		else
			pvals.push_back(bstrapHelp2(snp, IDmap, nboot, tobs));
	}

	return pvals;
}