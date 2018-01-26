#include "../RVS.h"
#include "CommonTestObject.h"
#include <future>

/*
RVS using asymptotic distribution for score test statistic. This functions includes
two association tests which differ in the estimation of the variance of score. It uses
var_case(E(G | D)) when rvs = false and var_case(G) when rvs = true.

Reference: http://www.ncbi.nlm.nih.gov/pubmed/22570057
Reference: http://www.ncbi.nlm.nih.gov/pubmed/24733292
@param CommonTestObject Test object.
@param rvs Indicates how to calculate variance
@return p-value
*/
double commonAsymptotic(CommonTestObject t, bool rvs) {

	double variance = 0;
	double score = 0;

	for (int i = 0; i < t.size(); i++) {
		score += t.getScore(i);
		variance += t.getVariance(i, rvs);
	}
	return chiSquareOneDOF(pow(score, 2) / variance);
}

/*
Uses RVS to test associaton by bootstrap, given phenotype, expected values of genotypes,
estimated genotype frequency and number of bootstrap iterations.

@param snps Vector of SNPs.
@param sample Vector with sample information.
@param nboot Number of bootstrap iterations.
@param stopEarly Stop bootstrapping when p-value looks big.
@param rvs Indicates how to calculate variance of score statistic.
@return Vector of p-values for the SNPs.
*/
double commonBootstrap(CommonTestObject t, int nboot, bool stopEarly, bool rvs) {
	int i;
	double variance = 0;
	double score = 0;

	for (int i = 0; i < t.size(); i++) {
		score += t.getScore(i);
		variance += t.getVariance(i, rvs);
	}

	double tobs = pow(score, 2) / variance;
	double tcount = 0;
	double average = 0;
	int bootcount = 0;

	for (int n = 0; n < nboot; n++) {
		bootcount++;

		variance = 0;
		score = 0;

		t.bootstrap();

		for (i = 0; i < t.size(); i++) {
			score += t.bootScore(i);
			variance += t.bootVariance(i);
		}

		if (tobs <= pow(score, 2) / variance)
			tcount++;

		average += score;

		//a = 5
		//theta = 0.6
		//delta = 0.400726
		//c = 0.0737224
		//p0 = 0.05

		if (stopEarly && bootcount > 10) {
			double pstar = 5 / ((bootcount + 0.0737224)*(1 + 0.0737224));
			if (tcount / bootcount > pstar) {
				std::cout << "early stop at ";
				std::cout << bootcount;
				std::cout << "\n ";
				break;
			}
		}
	}
	std::cout << "pval = ";
	std::cout << (tcount + 1) / (bootcount + 1);
	std::cout << "\n ";
	return (tcount + 1) / (bootcount + 1);
}

std::vector<double> runCommonTestParallel(Request &req, TestInput &input, int threadID, int nthreads) {
	
	MatrixXd X = input.X;
	VectorXd Y = input.Y;
	MatrixXd Z = input.Z;

	VectorXd G = input.G;
	std::map<int, int> readGroup = input.readGroup;
	MatrixXd P = input.P;

	std::vector<double> pvals;

	int i,j;

	std::vector<MatrixXd> x;
	std::vector<VectorXd> y;
	std::vector<MatrixXd> z;
	std::vector<int> rd;

	int ngroups = 1 + (int)G.maxCoeff();
	bool hasCovariates = input.hasCovariates();

	for (i = 0; i < ngroups; i++) {
		x.push_back(extractRows(X, G, i));
		y.push_back(extractRows(Y, G, i));

		if (hasCovariates)
			z.push_back(extractRows(Z, G, i));

		rd.push_back(readGroup[i]);
	}

	for (i = threadID; i < X.cols(); i++) {

		if (i % 25 == 0) {
			std::cout << "\n";
			std::cout << i;
			std::cout << "/";
			std::cout << X.cols();
			std::cout << " calculated.";
			std::cout << "\n";
		}

		std::vector<VectorXd> x_i;
		for (j = 0; j < ngroups; j++) 
			x_i.push_back(x[j].col(i));

		VectorXd X_i = X.col(i);
		VectorXd P_i = P.row(i);

		CommonTestObject testObject();
		if (hasCovariates) {
			CommonTestObject t(X_i, Y, Z, x_i, y, z, rd, P_i);
			
			if (req.nboot > 0)
				pvals.push_back(commonBootstrap(t, req.nboot, req.stopEarly, req.rvs));
			else
				pvals.push_back(commonAsymptotic(t, req.rvs));
		}
		else {
			CommonTestObject t(x_i, y, rd, P_i);

			if (req.nboot > 0)
				pvals.push_back(commonBootstrap(t, req.nboot, req.stopEarly, req.rvs));
			else
				pvals.push_back(commonAsymptotic(t, req.rvs));
		}
	}

	return pvals;
}


std::vector<double> runCommonTest(Request &req, TestInput &input) {

	int nthreads = req.nthreads;
	std::vector<std::future<std::vector<double>>> pvalsFuture;

	if (nthreads <= 1)
		return runCommonTestParallel(req, input, 1, 1);
	
	for (int i = 0; i < nthreads; i++) {
		pvalsFuture.push_back(std::async(std::launch::async,
			runCommonTestParallel,
			req, input, i, nthreads));
	}

	std::vector<std::vector<double>> pvalsFromThread;

	for (int i = 0; i < nthreads; i++) 
		pvalsFromThread.push_back(pvalsFuture[i].get());

	std::vector<double> pvals;
	
	int i = 0;
	while (true) {

		bool flag = false;
		for (int j = 0; j < nthreads; j++)
			if (pvalsFromThread[j].size() > i) {
				pvals.push_back(pvalsFromThread[j][i]);
				flag = true;
			}

		i++;

		if (!flag)
			break;
	}

	return pvals;
}



