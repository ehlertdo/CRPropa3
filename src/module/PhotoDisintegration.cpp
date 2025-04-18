#include "crpropa/module/PhotoDisintegration.h"
#include "crpropa/Units.h"
#include "crpropa/ParticleID.h"
#include "crpropa/ParticleMass.h"
#include "crpropa/Random.h"
#include "kiss/logger.h"

#include <cmath>
#include <limits>
#include <sstream>
#include <fstream>
#include <stdexcept>

namespace crpropa {

const double PhotoDisintegration::lgmin = 4;  // minimum log10(Lorentz-factor)
const double PhotoDisintegration::lgmax = 14; // maximum log10(Lorentz-factor)
const size_t PhotoDisintegration::nlg = 251;  // number of Lorentz-factor steps

PhotoDisintegration::PhotoDisintegration(ref_ptr<PhotonField> f, bool havePhotons, double limit) {
	setPhotonField(f);
	this->havePhotons = havePhotons;
	this->limit = limit;
}

void PhotoDisintegration::setPhotonField(ref_ptr<PhotonField> photonField) {
	this->photonField = photonField;
	std::string fname = photonField->getFieldName();
	setDescription("PhotoDisintegration: " + fname);
	initRate(getDataPath("Photodisintegration/rate_" + fname + ".txt"));
	initBranching(getDataPath("Photodisintegration/branching_" + fname + ".txt"));
	initPhotonEmission(getDataPath("Photodisintegration/photon_emission_" + fname.substr(0,3) + ".txt"));
}

void PhotoDisintegration::setHavePhotons(bool havePhotons) {
	this->havePhotons = havePhotons;
}

void PhotoDisintegration::setLimit(double limit) {
	this->limit = limit;
}

void PhotoDisintegration::initRate(std::string filename) {
	std::ifstream infile(filename.c_str());
	if (not infile.good())
		throw std::runtime_error("PhotoDisintegration: could not open file " + filename);

	// clear previously loaded interaction rates
	pdRate.clear();
	pdRate.resize(27 * 31);

	std::string line;
	while (std::getline(infile, line)) {
		if (line[0] == '#')
			continue;
		std::stringstream lineStream(line);

		int Z, N;
		lineStream >> Z;
		lineStream >> N;

		double r;
		for (size_t i = 0; i < nlg; i++) {
			lineStream >> r;
			pdRate[Z * 31 + N].push_back(r / Mpc);
		}
	}
	infile.close();
}

void PhotoDisintegration::initBranching(std::string filename) {
	std::ifstream infile(filename.c_str());
	if (not infile.good())
		throw std::runtime_error("PhotoDisintegration: could not open file " + filename);

	// clear previously loaded interaction rates
	pdBranch.clear();
	pdBranch.resize(27 * 31);

	std::string line;
	while (std::getline(infile, line)) {
		if (line[0] == '#')
			continue;

		std::stringstream lineStream(line);

		int Z, N;
		lineStream >> Z;
		lineStream >> N;

		Branch branch;
		lineStream >> branch.channel;

		double r;
		for (size_t i = 0; i < nlg; i++) {
			lineStream >> r;
			branch.branchingRatio.push_back(r);
		}

		pdBranch[Z * 31 + N].push_back(branch);
	}

	infile.close();
}

void PhotoDisintegration::initPhotonEmission(std::string filename) {
	std::ifstream infile(filename.c_str());
	if (not infile.good())
		throw std::runtime_error("PhotoDisintegration: could not open file " + filename);

	// clear previously loaded emission probabilities
	pdPhoton.clear();

	std::string line;
	while (std::getline(infile, line)) {
		if (line[0] == '#')
			continue;

		std::stringstream lineStream(line);

		int Z, N, Zd, Nd;
		lineStream >> Z;
		lineStream >> N;
		lineStream >> Zd;
		lineStream >> Nd;

		PhotonEmission em;
		lineStream >> em.energy;
		em.energy *= eV;

		double r;
		for (size_t i = 0; i < nlg; i++) {
			lineStream >> r;
			em.emissionProbability.push_back(r);
		}

		int key = Z * 1000000 + N * 10000 + Zd * 100 + Nd;
		if (pdPhoton.find(key) == pdPhoton.end()) {
			std::vector<PhotonEmission> emissions;
			pdPhoton[key] = emissions;
		}
		pdPhoton[key].push_back(em);
	}

	infile.close();
}

void PhotoDisintegration::process(Candidate *candidate) const {
	// execute the loop at least once for limiting the next step
	double step = candidate->getCurrentStep();
	do {
		// check if nucleus
		int id = candidate->current.getId();
		if (not isNucleus(id))
			return;

		int A = massNumber(id);
		int Z = chargeNumber(id);
		int N = A - Z;
		size_t idx = Z * 31 + N;

		// check if disintegration data available
		if ((Z > 26) or (N > 30))
			return;
		if (pdRate[idx].size() == 0)
			return;

		// check if in tabulated energy range
		double z = candidate->getRedshift();
		double lg = log10(candidate->current.getLorentzFactor() * (1 + z));
		if ((lg <= lgmin) or (lg >= lgmax))
			return;

		double rate = interpolateEquidistant(lg, lgmin, lgmax, pdRate[idx]);
		rate *= pow_integer<2>(1 + z) * photonField->getRedshiftScaling(z); // cosmological scaling, rate per comoving distance

		// radial dependence of the photon field
		double field_radial_scaling = photonField->getRadialScaling(candidate->current.getPosition().getR());
		rate *= field_radial_scaling;

		// check if interaction occurs in this step
		// otherwise limit next step to a fraction of the mean free path
		Random &random = Random::instance();
		double randDist = -log(random.rand()) / rate;
		if (step < randDist) {
			candidate->limitNextStep(limit / rate);
			return;
		}

		// select channel and interact
		const std::vector<Branch> &branches = pdBranch[idx];
		double cmp = random.rand();
		int l = round((lg - lgmin) / (lgmax - lgmin) * (nlg - 1)); // index of closest tabulation point
		size_t i = 0;
		while ((i < branches.size()) and (cmp > 0)) {
			cmp -= branches[i].branchingRatio[l];
			i++;
		}
		performInteraction(candidate, branches[i-1].channel);

		// repeat with remaining step
		step -= randDist;
	} while (step > 0);
}

void PhotoDisintegration::performInteraction(Candidate *candidate, int channel) const {
	KISS_LOG_DEBUG << "Photodisintegration::performInteraction. Channel " <<  channel << " on candidate " << candidate->getDescription(); 
	// parse disintegration channel
	int nNeutron = digit(channel, 100000);
	int nProton = digit(channel, 10000);
	int nH2 = digit(channel, 1000);
	int nH3 = digit(channel, 100);
	int nHe3 = digit(channel, 10);
	int nHe4 = digit(channel, 1);

	int dA = -nNeutron - nProton - 2 * nH2 - 3 * nH3 - 3 * nHe3 - 4 * nHe4;
	int dZ = -nProton - nH2 - nH3 - 2 * nHe3 - 2 * nHe4;

	int id = candidate->current.getId();
	int A = massNumber(id);
	int Z = chargeNumber(id);
	double EpA = candidate->current.getEnergy() / A;

	// create secondaries
	Random &random = Random::instance();
	Vector3d pos = random.randomInterpolatedPosition(candidate->previous.getPosition(), candidate->current.getPosition());
	try
	{
		for (size_t i = 0; i < nNeutron; i++)
			candidate->addSecondary(nucleusId(1, 0), EpA, pos, 1., interactionTag);
		for (size_t i = 0; i < nProton; i++)
			candidate->addSecondary(nucleusId(1, 1), EpA, pos, 1., interactionTag);
		for (size_t i = 0; i < nH2; i++)
			candidate->addSecondary(nucleusId(2, 1), EpA * 2, pos, 1., interactionTag);
		for (size_t i = 0; i < nH3; i++)
			candidate->addSecondary(nucleusId(3, 1), EpA * 3, pos, 1., interactionTag);
		for (size_t i = 0; i < nHe3; i++)
			candidate->addSecondary(nucleusId(3, 2), EpA * 3, pos, 1., interactionTag);
		for (size_t i = 0; i < nHe4; i++)
			candidate->addSecondary(nucleusId(4, 2), EpA * 4, pos, 1., interactionTag);


	// update particle
	  candidate->created = candidate->current;
		candidate->current.setId(nucleusId(A + dA, Z + dZ));
		candidate->current.setEnergy(EpA * (A + dA));
	}
	catch (std::runtime_error &e)
	{
		KISS_LOG_ERROR << "Something went wrong in the PhotoDisentigration\n" << "Please report this error on https://github.com/CRPropa/CRPropa3/issues including your simulation setup and the following random seed:\n" << Random::instance().getSeed_base64();
		throw;
	}

	if (not havePhotons)
		return;

	// create photons
	double z = candidate->getRedshift();
	double lg = log10(candidate->current.getLorentzFactor() * (1 + z));
	double lf = candidate->current.getLorentzFactor();

	int l = round((lg - lgmin) / (lgmax - lgmin) * (nlg - 1));  // index of closest tabulation point
	int key = Z*1e6 + (A-Z)*1e4 + (Z+dZ)*1e2 + (A+dA) - (Z+dZ);

	for (int i = 0; i < pdPhoton[key].size(); i++) {
		// check for random emission
		if (random.rand() > pdPhoton[key][i].emissionProbability[l])
			continue;

		// boost to lab frame
		double cosTheta = 2 * random.rand() - 1;
		double E = pdPhoton[key][i].energy * lf * (1 - cosTheta);
		candidate->addSecondary(22, E, pos, 1., interactionTag);
	}
}

double PhotoDisintegration::lossLength(int id, double gamma, double z) {
	// check if nucleus
	if (not (isNucleus(id)))
		return std::numeric_limits<double>::max();

	int A = massNumber(id);
	int Z = chargeNumber(id);
	int N = A - Z;
	size_t idx = Z * 31 + N;

	// check if disintegration data available
	if ((Z > 26) or (N > 30))
		return std::numeric_limits<double>::max();
	const std::vector<double> &rate = pdRate[idx];
	if (rate.size() == 0)
		return std::numeric_limits<double>::max();

	// check if in tabulated energy range
	double lg = log10(gamma * (1 + z));
	if ((lg <= lgmin) or (lg >= lgmax))
		return std::numeric_limits<double>::max();

	// total interaction rate
	double lossRate = interpolateEquidistant(lg, lgmin, lgmax, rate);

	// comological scaling, rate per physical distance
	lossRate *= pow_integer<3>(1 + z) * photonField->getRedshiftScaling(z);

	// average number of nucleons lost for all disintegration channels
	double avg_dA = 0;
	const std::vector<Branch> &branches = pdBranch[idx];
	for (size_t i = 0; i < branches.size(); i++) {
		int channel = branches[i].channel;
		int dA = 0;
		dA += 1 * digit(channel, 100000);
		dA += 1 * digit(channel, 10000);
		dA += 2 * digit(channel, 1000);
		dA += 3 * digit(channel, 100);
		dA += 3 * digit(channel, 10);
		dA += 4 * digit(channel, 1);

		double br = interpolateEquidistant(lg, lgmin, lgmax, branches[i].branchingRatio);
		avg_dA += br * dA;
	}

	lossRate *= avg_dA / A;
	return 1 / lossRate;
}

void PhotoDisintegration::setInteractionTag(std::string tag) {
	interactionTag = tag;
}

std::string PhotoDisintegration::getInteractionTag() const {
	return interactionTag;
}

} // namespace crpropa
