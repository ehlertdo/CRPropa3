#include "crpropa/Candidate.h"
#include "crpropa/Units.h"
#include "crpropa/ParticleID.h"
#include "crpropa/PhotonBackground.h"
#include "crpropa/module/ElectronPairProduction.h"
#include "crpropa/module/NuclearDecay.h"
#include "crpropa/module/PhotoDisintegration.h"
#include "crpropa/module/ElasticScattering.h"
#include "crpropa/module/PhotoPionProduction.h"
#include "crpropa/module/Redshift.h"
#include "crpropa/module/EMPairProduction.h"
#include "crpropa/module/EMDoublePairProduction.h"
#include "crpropa/module/EMTripletPairProduction.h"
#include "crpropa/module/EMInverseComptonScattering.h"
#include "crpropa/module/SynchrotronRadiation.h"
#include "gtest/gtest.h"

#include <fstream>

namespace crpropa {

// ElectronPairProduction -----------------------------------------------------
TEST(ElectronPairProduction, allBackgrounds) {
	// Test if interaction data files are loaded.
	ref_ptr<PhotonField> cmb = new CMB();
	ElectronPairProduction epp(cmb);
	ref_ptr<PhotonField> irb = new IRB_Kneiske04();
	epp.setPhotonField(irb);
	irb = new IRB_Stecker05();
	epp.setPhotonField(irb);
	irb = new IRB_Franceschini08();
	epp.setPhotonField(irb);
	irb = new IRB_Finke10();
	epp.setPhotonField(irb);
	irb = new IRB_Dominguez11();
	epp.setPhotonField(irb);
	irb = new IRB_Gilmore12();
	epp.setPhotonField(irb);
	irb = new IRB_Stecker16_upper();
	epp.setPhotonField(irb);
	irb = new IRB_Stecker16_lower();
	epp.setPhotonField(irb);
    irb = new IRB_Finke22();
	epp.setPhotonField(irb);
}

TEST(ElectronPairProduction, energyDecreasing) {
	// Test if energy loss occurs for protons with energies from 1e15 - 1e23 eV.
	Candidate c;
	c.setCurrentStep(2 * Mpc);
	c.current.setId(nucleusId(1, 1)); // proton

	ref_ptr<PhotonField> cmb = new CMB();
	ElectronPairProduction epp1(cmb);
	for (int i = 0; i < 80; i++) {
		double E = pow(10, 15 + i * 0.1) * eV;
		c.current.setEnergy(E);
		epp1.process(&c);
		EXPECT_LE(c.current.getEnergy(), E);
	}

	ref_ptr<PhotonField> irb = new IRB_Kneiske04();
	ElectronPairProduction epp2(irb);
	for (int i = 0; i < 80; i++) {
		double E = pow(10, 15 + i * 0.1) * eV;
		c.current.setEnergy(E);
		epp2.process(&c);
		EXPECT_LE(c.current.getEnergy(), E);
	}
}

TEST(ElectronPairProduction, belowEnergyTreshold) {
	// Test if nothing happens below 1e15 eV.
	ref_ptr<PhotonField> cmb = new CMB();
	ElectronPairProduction epp(cmb);
	Candidate c(nucleusId(1, 1), 1E14 * eV);
	epp.process(&c);
	EXPECT_DOUBLE_EQ(1E14 * eV, c.current.getEnergy());
}

TEST(ElectronPairProduction, thisIsNotNucleonic) {
	// Test if non-nuclei are skipped.
	ref_ptr<PhotonField> cmb = new CMB();
	ElectronPairProduction epp(cmb);
	Candidate c(11, 1E20 * eV);  // electron
	epp.process(&c);
	EXPECT_DOUBLE_EQ(1E20 * eV, c.current.getEnergy());
}

TEST(ElectronPairProduction, valuesCMB) {
	// Test if energy loss corresponds to the data table.
	std::vector<double> x;
	std::vector<double> y;
	std::ifstream infile(getDataPath("pair_CMB.txt").c_str());
	while (infile.good()) {
		if (infile.peek() != '#') {
			double a, b;
			infile >> a >> b;
			if (infile) {
				x.push_back(a * eV);
				y.push_back(b * eV / Mpc);
			}
		}
		infile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	infile.close();

	Candidate c;
	c.setCurrentStep(1 * Mpc);
	c.current.setId(nucleusId(1, 1)); // proton
	ref_ptr<PhotonField> cmb = new CMB();

	ElectronPairProduction epp(cmb);
	for (int i = 0; i < x.size(); i++) {
		c.current.setEnergy(x[i]);
		epp.process(&c);
		double dE = x[i] - c.current.getEnergy();
		double dE_table = y[i] * 1 * Mpc;
		EXPECT_NEAR(dE_table, dE, 1e-12);
	}
}

TEST(ElectronPairProduction, interactionTag) {
	
	ref_ptr<PhotonField> cmb = new CMB();
	ElectronPairProduction epp(cmb);
	
	// test the default interaction tag
	EXPECT_TRUE(epp.getInteractionTag() == "EPP");

	// test changing the interaction tag
	epp.setInteractionTag("myTag");
	EXPECT_TRUE(epp.getInteractionTag() == "myTag");

	// test the tag of produced secondaries
	Candidate c;
	c.setCurrentStep(1 * Gpc);
	c.current.setId(nucleusId(1,1));
	c.current.setEnergy(100 * EeV);
	epp.setHaveElectrons(true);
	epp.process(&c);
	
	std::string secondaryTag = c.secondaries[0] -> getTagOrigin();
	EXPECT_TRUE(secondaryTag == "myTag");
}

TEST(ElectronPairProduction, valuesIRB) {
	// Test if energy loss corresponds to the data table.
	std::vector<double> x;
	std::vector<double> y;
	std::ifstream infile(getDataPath("pairIRB.txt").c_str());
	while (infile.good()) {
		if (infile.peek() != '#') {
			double a, b;
			infile >> a >> b;
			if (infile) {
				x.push_back(a * eV);
				y.push_back(b * eV / Mpc);
			}
		}
		infile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	infile.close();

	Candidate c;
	c.setCurrentStep(1 * Mpc);
	c.current.setId(nucleusId(1, 1)); // proton
	ref_ptr<PhotonField> irb = new IRB_Kneiske04();

	ElectronPairProduction epp(irb);
	for (int i = 0; i < x.size(); i++) {
		c.current.setEnergy(x[i]);
		epp.process(&c);
		double dE = x[i] - c.current.getEnergy();
		double dE_table = y[i] * 1 * Mpc;
		EXPECT_NEAR(dE, dE_table, 1e-12);
	}
}

// NuclearDecay ---------------------------------------------------------------
TEST(NuclearDecay, scandium44) {
	// Test beta+ decay of 44Sc to 44Ca.
	// This test can stochastically fail.
	NuclearDecay d(true, true);
	Candidate c(nucleusId(44, 21), 1E18 * eV);
	c.setCurrentStep(100 * Mpc);
	double gamma = c.current.getLorentzFactor();
	d.process(&c);
	
	// expected decay product: 44Ca
	EXPECT_EQ(nucleusId(44, 20), c.current.getId());

	// expect Lorentz factor to be conserved
	EXPECT_DOUBLE_EQ(gamma, c.current.getLorentzFactor());
	
	// expect at least two secondaries: positron + electron neutrino
	EXPECT_GE(c.secondaries.size(), 2);
}

TEST(NuclearDecay, lithium4) {
	// Test proton dripping of Li-4 to He-3
	// This test can stochastically fail
	NuclearDecay d;
	Candidate c(nucleusId(4, 3), 4 * EeV);
	c.setCurrentStep(100 * Mpc);
	d.process(&c);
	
	// expected decay product: He-3
	EXPECT_EQ(nucleusId(3, 2), c.current.getId());

	// expected secondary: proton
	EXPECT_EQ(1, c.secondaries.size());
	Candidate c1 = *c.secondaries[0];
	EXPECT_EQ(nucleusId(1, 1), c1.current.getId());
	EXPECT_EQ(1 * EeV, c1.current.getEnergy());
}

TEST(NuclearDecay, helium5) {
	// Test neutron dripping of He-5 to He-4.
	// This test can stochastically fail.
	NuclearDecay d;
	Candidate c(nucleusId(5, 2), 5 * EeV);
	c.setCurrentStep(100 * Mpc);
	d.process(&c);

	// expected primary: He-4
	EXPECT_EQ(nucleusId(4, 2), c.current.getId());
	EXPECT_EQ(4, c.current.getEnergy() / EeV);
	
	// expected secondary: neutron
	Candidate c2 = *c.secondaries[0];
	EXPECT_EQ(nucleusId(1, 0), c2.current.getId());
	EXPECT_EQ(1, c2.current.getEnergy() / EeV);
}

TEST(NuclearDecay, limitNextStep) {
	// Test if next step is limited in case of a neutron.
	NuclearDecay decay;
	Candidate c(nucleusId(1, 0), 10 * EeV);
	c.setNextStep(std::numeric_limits<double>::max());
	decay.process(&c);
	EXPECT_LT(c.getNextStep(), std::numeric_limits<double>::max());
}

TEST(NuclearDecay, allChannelsWorking) {
	// Test if all nuclear decays are working.
	NuclearDecay d;
	Candidate c;

	std::ifstream infile(getDataPath("nuclear_decay.txt").c_str());
	while (infile.good()) {
		if (infile.peek() != '#') {
			int Z, N, channel, foo;
			infile >> Z >> N >> channel >> foo;
			c.current.setId(nucleusId(Z + N, Z));
			c.current.setEnergy(80 * EeV);
			d.performInteraction(&c, channel);
		}
		infile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	infile.close();
}

TEST(NuclearDecay, secondaries) {
	// Test if all types of secondaries are produced.
	NuclearDecay d;
	d.setHaveElectrons(true);
	d.setHaveNeutrinos(true);
	d.setHavePhotons(true);
	Candidate c;

	// He-8 --> Li-8 + e- + neutrino
	// additional photon emitted with 84% probability
	// --> expect at least 1 photon out of 10 decays
	for (int i = 0; i < 10; ++i) {
		c.current.setId(nucleusId(8, 2));
		c.current.setEnergy(5 * EeV);
		d.performInteraction(&c, 10000);
	}

	// count number of secondaries
	size_t nElectrons = 0;
	size_t nNeutrinos = 0;
	size_t nPhotons = 0;

	for(size_t i = 0; i < c.secondaries.size(); ++i) {
		int id = (*c.secondaries[i]).current.getId();
		if (id == 22) nPhotons++;
		if (id == 11) nElectrons++;
		if (id == -12) nNeutrinos++;
	}

	EXPECT_EQ(nElectrons, 10);
	EXPECT_EQ(nNeutrinos, 10);
	EXPECT_GE(nPhotons, 1);
}

TEST(NuclearDecay, thisIsNotNucleonic) {
	// Test if nothing happens to an electron
	NuclearDecay decay;
	Candidate c(11, 10 * EeV);
	c.setNextStep(std::numeric_limits<double>::max());
	decay.process(&c);
	EXPECT_EQ(11, c.current.getId());
	EXPECT_EQ(10 * EeV, c.current.getEnergy());
}

TEST(NuclearDecay, interactionTag) {
	// test default interaction tag
	NuclearDecay decay;
	EXPECT_TRUE(decay.getInteractionTag() == "ND");

	// test secondary tag
	decay.setHaveElectrons(true);
	Candidate c(nucleusId(8,2), 5 * EeV);
	decay.performInteraction(&c, 10000);
	EXPECT_TRUE(c.secondaries[0] -> getTagOrigin() == "ND");

	// test custom tags
	decay.setInteractionTag("myTag");
	EXPECT_TRUE(decay.getInteractionTag() == "myTag");
}

// PhotoDisintegration --------------------------------------------------------
TEST(PhotoDisintegration, allBackgrounds) {
	// Test if interaction data files are loaded.
	ref_ptr<PhotonField> cmb = new CMB();
	PhotoDisintegration pd(cmb);
	ref_ptr<PhotonField> irb = new IRB_Kneiske04();
	pd.setPhotonField(irb);
	ref_ptr<PhotonField> urb = new URB_Protheroe96();
	pd.setPhotonField(urb);
	irb = new IRB_Stecker05();
	pd.setPhotonField(irb);
	irb = new IRB_Franceschini08();
	pd.setPhotonField(irb);
	irb = new IRB_Finke10();
	pd.setPhotonField(irb);
	irb = new IRB_Dominguez11();
	pd.setPhotonField(irb);
	irb = new IRB_Gilmore12();
	pd.setPhotonField(irb);
	irb = new IRB_Stecker16_upper();
	pd.setPhotonField(irb);
	irb = new IRB_Stecker16_lower();
	pd.setPhotonField(irb);
    irb = new IRB_Finke22();
	pd.setPhotonField(irb);
	urb = new URB_Nitu21();
	pd.setPhotonField(urb);
}

TEST(PhotoDisintegration, carbon) {
	// Test if a 100 EeV C-12 nucleus photo-disintegrates (at least once) over a distance of 1 Gpc.
	// This test can stochastically fail.
	ref_ptr<PhotonField> cmb = new CMB();
	PhotoDisintegration pd(cmb);
	Candidate c;
	int id = nucleusId(12, 6);
	c.current.setId(id);
	c.current.setEnergy(100 * EeV);
	c.setCurrentStep(1000 * Mpc);
	pd.process(&c);

	EXPECT_TRUE(c.current.getEnergy() < 100 * EeV);
	// energy loss
	EXPECT_TRUE(c.secondaries.size() > 0);
	// secondaries produced

	double E = c.current.getEnergy();
	id = c.current.getId();
	int A = massNumber(id);
	int Z = chargeNumber(id);

	for (int i = 0; i < c.secondaries.size(); i++) {
		E += (*c.secondaries[i]).current.getEnergy();
		id = (*c.secondaries[i]).current.getId();
		A += massNumber(id);
		Z += chargeNumber(id);
	}
	EXPECT_EQ(12, A);
	// nucleon number conserved
	EXPECT_EQ(6, Z);
	// proton number conserved
	EXPECT_DOUBLE_EQ(100 * EeV, E);
	// energy conserved
}

TEST(PhotoDisintegration, iron) {
	// Test if a 200 EeV Fe-56 nucleus photo-disintegrates (at least once) over a distance of 1 Gpc.
	// This test can stochastically fail.
	ref_ptr<PhotonField> irb = new IRB_Kneiske04();
	PhotoDisintegration pd(irb);
	Candidate c;
	int id = nucleusId(56, 26);
	c.current.setId(id);
	c.current.setEnergy(200 * EeV);
	c.setCurrentStep(1000 * Mpc);
	pd.process(&c);

	// expect energy loss
	EXPECT_TRUE(c.current.getEnergy() < 200 * EeV);
	
	// expect secondaries produced
	EXPECT_TRUE(c.secondaries.size() > 0);

	double E = c.current.getEnergy();
	id = c.current.getId();
	int A = massNumber(id);
	int Z = chargeNumber(id);

	for (int i = 0; i < c.secondaries.size(); i++) {
		E += (*c.secondaries[i]).current.getEnergy();
		id = (*c.secondaries[i]).current.getId();
		A += massNumber(id);
		Z += chargeNumber(id);
	}

	// nucleon number conserved
	EXPECT_EQ(56, A);
	
	// proton number conserved (no decay active)
	EXPECT_EQ(26, Z);
	
	// energy conserved
	EXPECT_DOUBLE_EQ(200 * EeV, E);
}

TEST(PhotoDisintegration, thisIsNotNucleonic) {
	// Test that nothing happens to an electron.
	ref_ptr<PhotonField> cmb = new CMB();
	PhotoDisintegration pd(cmb);
	Candidate c;
	c.setCurrentStep(1 * Mpc);
	c.current.setId(11); // electron
	c.current.setEnergy(10 * EeV);
	pd.process(&c);
	EXPECT_EQ(11, c.current.getId());
	EXPECT_EQ(10 * EeV, c.current.getEnergy());
}

TEST(PhotoDisintegration, limitNextStep) {
	// Test if the interaction limits the next propagation step.
	ref_ptr<PhotonField> cmb = new CMB();
	PhotoDisintegration pd(cmb);
	Candidate c;
	c.setNextStep(std::numeric_limits<double>::max());
	c.current.setId(nucleusId(4, 2));
	c.current.setEnergy(200 * EeV);
	pd.process(&c);
	EXPECT_LT(c.getNextStep(), std::numeric_limits<double>::max());
}

TEST(PhotoDisintegration, allIsotopes) {
	// Test if all isotopes are handled.
	ref_ptr<PhotonField> cmb = new CMB();
	PhotoDisintegration pd1(cmb);
	ref_ptr<PhotonField> irb = new IRB_Kneiske04();
	PhotoDisintegration pd2(irb);
	Candidate c;
	c.setCurrentStep(10 * Mpc);

	for (int Z = 1; Z <= 26; Z++) {
		for (int N = 1; N <= 30; N++) {

			c.current.setId(nucleusId(Z + N, Z));
			c.current.setEnergy(80 * EeV);
			pd1.process(&c);

			c.current.setId(nucleusId(Z + N, Z));
			c.current.setEnergy(80 * EeV);
			pd2.process(&c);
		}
	}
}

TEST(Photodisintegration, updateParticleParentProperties) { // Issue: #204
	ref_ptr<PhotonField> cmb = new CMB();
	PhotoDisintegration pd(cmb);

	Candidate c(nucleusId(56,26), 500 * EeV, Vector3d(1 * Mpc, 0, 0));

	pd.performInteraction(&c, 1);
	// the candidates parent is the original particle
	EXPECT_EQ(c.created.getId(), nucleusId(56,26));

	pd.performInteraction(&c, 1);
	// now it has to be changed
	EXPECT_NE(c.created.getId(), nucleusId(56,26));
}

TEST(PhotoDisintegration, interactionTag) {
	PhotoDisintegration pd(new CMB());

	// test default interactionTag
	EXPECT_TRUE(pd.getInteractionTag() == "PD");

	// test secondary tag
	pd.setHavePhotons(true);
	Candidate c(nucleusId(56,26), 500 * EeV);
	c.setCurrentStep(1 * Gpc);
	pd.process(&c);
	EXPECT_TRUE(c.secondaries[0] -> getTagOrigin() == "PD");

	// test custom tag
	pd.setInteractionTag("myTag");
	EXPECT_TRUE(pd.getInteractionTag() == "myTag");
}

// ElasticScattering ----------------------------------------------------------
TEST(ElasticScattering, allBackgrounds) {
	// Test if interaction data files are loaded.
	ref_ptr<PhotonField> cmb = new CMB();
	ElasticScattering scattering(cmb);
	ref_ptr<PhotonField> irb = new IRB_Kneiske04();
	scattering.setPhotonField(irb);
	ref_ptr<PhotonField> urb = new URB_Nitu21();
	scattering.setPhotonField(urb);
}

TEST(ElasticScattering, secondaries) {
	// Test the creation of cosmic ray photons.
	// This test can stochastically fail.
	ref_ptr<PhotonField> cmb = new CMB();
	ElasticScattering scattering(cmb);
	Candidate c;
	int id = nucleusId(12, 6);
	c.current.setId(id);
	c.current.setEnergy(200 * EeV);
	c.setCurrentStep(400 * Mpc);
	scattering.process(&c);

	EXPECT_GT(c.secondaries.size(), 0);

	for (int i = 0; i < c.secondaries.size(); i++) {
		int id = (*c.secondaries[i]).current.getId();
		EXPECT_EQ(id, 22);
		double energy = (*c.secondaries[i]).current.getEnergy();
		EXPECT_GT(energy, 0);
		EXPECT_LT(energy, 200 * EeV);
	}
}

// PhotoPionProduction --------------------------------------------------------
TEST(PhotoPionProduction, allBackgrounds) {
	// Test if all interaction data files can be loaded.
	ref_ptr<PhotonField> cmb = new CMB();
	PhotoPionProduction ppp(cmb);
	ref_ptr<PhotonField> irb = new IRB_Kneiske04();
	ppp.setPhotonField(irb);
	irb = new IRB_Stecker05();
	ppp.setPhotonField(irb);
	irb = new IRB_Franceschini08();
	ppp.setPhotonField(irb);
	irb = new IRB_Finke10();
	ppp.setPhotonField(irb);
	irb = new IRB_Dominguez11();
	ppp.setPhotonField(irb);
	irb = new IRB_Gilmore12();
	ppp.setPhotonField(irb);
	irb = new IRB_Stecker16_upper();
	ppp.setPhotonField(irb);
	irb = new IRB_Stecker16_lower();
	ppp.setPhotonField(irb);
    irb = new IRB_Finke22();
	ppp.setPhotonField(irb);
	ref_ptr<PhotonField> urb = new URB_Protheroe96();
	ppp.setPhotonField(urb);
	urb = new URB_Nitu21();
	ppp.setPhotonField(urb);
}

TEST(PhotoPionProduction, proton) {
	// Test photopion interaction for 100 EeV proton.
	// This test can stochastically fail.
	ref_ptr<PhotonField> cmb = new CMB();
	PhotoPionProduction ppp(cmb);
	Candidate c(nucleusId(1, 1), 100 * EeV);
	c.setCurrentStep(1000 * Mpc);
	ppp.process(&c);

	// expect energy loss
	EXPECT_LT(c.current.getEnergy(), 100. * EeV);

	// expect nucleon number conservation
	EXPECT_EQ(1, massNumber(c.current.getId()));

	// expect no (nucleonic) secondaries
	EXPECT_EQ(0, c.secondaries.size());
}

TEST(PhotoPionProduction, helium) {
	// Test photo-pion interaction for 400 EeV He nucleus.
	// This test can stochastically fail.
	ref_ptr<PhotonField> cmb = new CMB();
	PhotoPionProduction ppp(cmb);
	Candidate c;
	c.current.setId(nucleusId(4, 2));
	c.current.setEnergy(400. * EeV);
	c.setCurrentStep(1000 * Mpc);
	ppp.process(&c);
	EXPECT_LT(c.current.getEnergy(), 400. * EeV);
	int id = c.current.getId();
	EXPECT_TRUE(massNumber(id) < 4);
	EXPECT_TRUE(c.secondaries.size() > 0);
}

TEST(PhotoPionProduction, thisIsNotNucleonic) {
	// Test if nothing happens to an electron.
	ref_ptr<PhotonField> cmb = new CMB();
	PhotoPionProduction ppp(cmb);
	Candidate c;
	c.current.setId(11); // electron
	c.current.setEnergy(10 * EeV);
	c.setCurrentStep(100 * Mpc);
	ppp.process(&c);
	EXPECT_EQ(11, c.current.getId());
	EXPECT_EQ(10 * EeV, c.current.getEnergy());
}

TEST(PhotoPionProduction, limitNextStep) {
	// Test if the interaction limits the next propagation step.
	ref_ptr<PhotonField> cmb = new CMB();
	PhotoPionProduction ppp(cmb);
	Candidate c(nucleusId(1, 1), 200 * EeV);
	c.setNextStep(std::numeric_limits<double>::max());
	ppp.process(&c);
	EXPECT_LT(c.getNextStep(), std::numeric_limits<double>::max());
}

TEST(PhotoPionProduction, secondaries) {
	// Test photo-pion interaction for 100 EeV proton.
	// This test can stochastically fail.
	ref_ptr<PhotonField> cmb = new CMB();
	PhotoPionProduction ppp(cmb, true, true, true);
	Candidate c(nucleusId(1, 1), 100 * EeV);
	c.setCurrentStep(1000 * Mpc);
	ppp.process(&c);
	// there should be secondaries
	EXPECT_GT(c.secondaries.size(), 1);
}

TEST(PhotoPionProduction, sampling) {
	// Specific test of photon sampling of photo-pion production
	// by testing the calculated pEpsMax for CMB(), also indirectly
	// testing epsMinInteraction and logSampling (default).
	ref_ptr<PhotonField> cmb = new CMB(); //create CMB instance
	double energy = 1.e10; //1e10 GeV
	bool onProton = true; //proton
	double z = 0; //no redshift
	PhotoPionProduction ppp(cmb, true, true, true);
	double correctionFactor = ppp.getCorrectionFactor(); //get current correctionFactor
	double epsMin = std::max(cmb -> getMinimumPhotonEnergy(z) / eV, 0.00710614); // 0.00710614 = epsMinInteraction(onProton,energy)
	double epsMax = cmb -> getMaximumPhotonEnergy(z) / eV;
	Candidate c(nucleusId(1, 1), 100 * EeV);
	double pEpsMax = ppp.probEpsMax(&c, onProton, energy, z, epsMin, epsMax) / correctionFactor;
	EXPECT_DOUBLE_EQ(pEpsMax,132673934934.922);
}

TEST(PhotoPionProduction, interactionTag) {
	PhotoPionProduction ppp(new CMB());

	// test default interactionTag
	EXPECT_TRUE(ppp.getInteractionTag() == "PPP");

	// test secondary tag
	ppp.setHavePhotons(true);
	Candidate c(nucleusId(1,1), 100 * EeV);
	for(int i = 0; i <10; i++) 
		ppp.performInteraction(&c, true);
	EXPECT_TRUE(c.secondaries[0] -> getTagOrigin() == "PPP");

	// test custom interactionTag
	ppp.setInteractionTag("myTag");
	EXPECT_TRUE(ppp.getInteractionTag() == "myTag");
}

// Redshift -------------------------------------------------------------------
TEST(Redshift, simpleTest) {
	// Test if redshift is decreased and adiabatic energy loss is applied.
	Redshift redshift;

	Candidate c;
	c.setRedshift(0.024);
	c.current.setEnergy(100 * EeV);
	c.setCurrentStep(1 * Mpc);

	redshift.process(&c);
	EXPECT_GT(0.024, c.getRedshift()); // expect redshift decrease
	EXPECT_GT(100, c.current.getEnergy() / EeV); // expect energy loss
}

TEST(Redshift, limitRedshiftDecrease) {
	// Test if the redshift decrease is limited to z_updated >= 0.
	Redshift redshift;

	Candidate c;
	c.setRedshift(0.024); // roughly corresponds to 100 Mpc
	c.setCurrentStep(150 * Mpc);

	redshift.process(&c);
	EXPECT_DOUBLE_EQ(0, c.getRedshift());
}

// EMPairProduction -----------------------------------------------------------
TEST(EMPairProduction, allBackgrounds) {
	// Test if interaction data files are loaded.
	ref_ptr<PhotonField> cmb = new CMB();
	EMPairProduction em(cmb);
	ref_ptr<PhotonField> ebl = new IRB_Kneiske04();
	em.setPhotonField(ebl);
	ref_ptr<PhotonField> urb = new URB_Protheroe96();
	em.setPhotonField(urb);
	ebl = new IRB_Stecker05();
	em.setPhotonField(ebl);
	ebl = new IRB_Franceschini08();
	em.setPhotonField(ebl);
	ebl = new IRB_Finke10();
	em.setPhotonField(ebl);
	ebl = new IRB_Dominguez11();
	em.setPhotonField(ebl);
	ebl = new IRB_Gilmore12();
	em.setPhotonField(ebl);
	ebl = new IRB_Stecker16_upper();
	em.setPhotonField(ebl);
	ebl = new IRB_Stecker16_lower();
	em.setPhotonField(ebl);
	ebl = new IRB_Finke22();
	em.setPhotonField(ebl);
	urb = new URB_Fixsen11();
	em.setPhotonField(urb);
	urb = new URB_Nitu21();
	em.setPhotonField(urb);
}

TEST(EMPairProduction, limitNextStep) {
	// Test if the interaction limits the next propagation step.
	ref_ptr<PhotonField> cmb = new CMB();
	EMPairProduction m(cmb);
	Candidate c(22, 1E17 * eV);
	c.setNextStep(std::numeric_limits<double>::max());
	m.process(&c);
	EXPECT_LT(c.getNextStep(), std::numeric_limits<double>::max());
}

TEST(EMPairProduction, secondaries) {
	// Test if secondaries are correctly produced.
	ref_ptr<PhotonField> cmb = new CMB();
	ref_ptr<PhotonField> irb = new IRB_Saldana21();
	ref_ptr<PhotonField> urb = new URB_Nitu21();
	EMPairProduction m(cmb);
	m.setHaveElectrons(true);
	m.setThinning(0.);

	std::vector<ref_ptr<PhotonField>> fields;
	fields.push_back(cmb);
	fields.push_back(irb);
	fields.push_back(urb);

	// loop over photon backgrounds
	for (int f = 0; f < fields.size(); f++) {
		m.setPhotonField(fields[f]);
		for (int i = 0; i < 140; i++) { // loop over energies Ep = (1e10 - 1e23) eV
			double Ep = pow(10, 9.05 + 0.1 * i) * eV;
			Candidate c(22, Ep);
			c.setCurrentStep(1e10 * Mpc);

			m.process(&c);

			// pass if no interaction has ocurred (no tabulated rates)
			if (c.isActive())
				continue;
			
			// expect 2 secondaries
			EXPECT_EQ(c.secondaries.size(), 2);

			// expect electron / positron with energies 0 < E < Ephoton
			double Etot = 0;
			for (int j = 0; j < c.secondaries.size(); j++) {
				Candidate s = *c.secondaries[j];
				EXPECT_EQ(abs(s.current.getId()), 11);
				EXPECT_GT(s.current.getEnergy(), 0);
				EXPECT_LT(s.current.getEnergy(), Ep);
				Etot += s.current.getEnergy();
			}

			// test energy conservation
			EXPECT_DOUBLE_EQ(Ep, Etot);
		}
	}
}

TEST(EMPairProduction, interactionTag) {
	EMPairProduction m(new CMB());

	// test default interactionTag
	EXPECT_TRUE(m.getInteractionTag() == "EMPP");

	// test secondary tag
	m.setHaveElectrons(true);
	Candidate c(22, 1 * EeV);
	m.performInteraction(&c);
	EXPECT_TRUE(c.secondaries[0] -> getTagOrigin() == "EMPP");

	// test custom tag
	m.setInteractionTag("myTag");
	EXPECT_TRUE(m.getInteractionTag() == "myTag");
}

// EMDoublePairProduction -----------------------------------------------------
TEST(EMDoublePairProduction, allBackgrounds) {
	// Test if interaction data files are loaded.
	ref_ptr<PhotonField> cmb = new CMB();
	EMDoublePairProduction em(cmb);
	ref_ptr<PhotonField> ebl = new IRB_Kneiske04();
	em.setPhotonField(ebl);
	ref_ptr<PhotonField> urb = new URB_Protheroe96();
	em.setPhotonField(urb);
	ebl = new IRB_Stecker05();
	em.setPhotonField(ebl);
	ebl = new IRB_Franceschini08();
	em.setPhotonField(ebl);
	ebl = new IRB_Finke10();
	em.setPhotonField(ebl);
	ebl = new IRB_Dominguez11();
	em.setPhotonField(ebl);
	ebl = new IRB_Gilmore12();
	em.setPhotonField(ebl);
	ebl = new IRB_Stecker16_upper();
	em.setPhotonField(ebl);
	ebl = new IRB_Stecker16_lower();
	em.setPhotonField(ebl);
	ebl = new IRB_Finke22();
	em.setPhotonField(ebl);
	urb = new URB_Fixsen11();
	em.setPhotonField(urb);
	urb = new URB_Nitu21();
	em.setPhotonField(urb);
}

TEST(EMDoublePairProduction, limitNextStep) {
	// Test if the interaction limits the next propagation step.
	ref_ptr<PhotonField> cmb = new CMB();
	EMDoublePairProduction m(cmb);
	Candidate c(22, 1E17 * eV);
	c.setNextStep(std::numeric_limits<double>::max());
	m.process(&c);
	EXPECT_LT(c.getNextStep(), std::numeric_limits<double>::max());
}

TEST(EMDoublePairProduction, secondaries) {
	// Test if secondaries are correctly produced.
	ref_ptr<PhotonField> cmb = new CMB();
	ref_ptr<PhotonField> irb = new IRB_Saldana21();
	ref_ptr<PhotonField> urb = new URB_Nitu21();
	EMPairProduction m(cmb);
	m.setHaveElectrons(true);
	m.setThinning(0.);

	std::vector<ref_ptr<PhotonField>> fields;
	fields.push_back(cmb);
	fields.push_back(irb);
	fields.push_back(urb);

	// loop over photon backgrounds
	for (int f = 0; f < fields.size(); f++) {
		m.setPhotonField(fields[f]);
		
		// loop over energies Ep = (1e9 - 1e23) eV
		for (int i = 0; i < 140; i++) {
			double Ep = pow(10, 9.05 + 0.1 * i) * eV;
			Candidate c(22, Ep);
			c.setCurrentStep(1e4 * Mpc); // use lower value so that the test can run faster
			m.process(&c);

			// pass if no interaction has occured (no tabulated rates)
			if (c.isActive())
				continue;
			
			// expect 2 secondaries (only one pair is considered)
			EXPECT_EQ(c.secondaries.size(), 2);

			// expect electron / positron with energies 0 < E < Ephoton
			double Etot = 0;
			for (int j = 0; j < c.secondaries.size(); j++) {
				Candidate s = *c.secondaries[j];
				EXPECT_EQ(abs(s.current.getId()), 11);
				EXPECT_GT(s.current.getEnergy(), 0);
				EXPECT_LT(s.current.getEnergy(), Ep);
				Etot += s.current.getEnergy();
			}

			// test energy conservation
			EXPECT_NEAR(Ep, Etot, 1E-9);
		}
	}
}

TEST(EMDoublePairProduction, interactionTag) {
	EMDoublePairProduction m(new CMB());

	// test default interactionTag
	EXPECT_TRUE(m.getInteractionTag() == "EMDP");

	// test secondary tag
	m.setHaveElectrons(true);
	Candidate c(22, 1 * EeV);
	m.performInteraction(&c);
	EXPECT_TRUE(c.secondaries[0] -> getTagOrigin() == "EMDP");

	// test custom tag
	m.setInteractionTag("myTag");
	EXPECT_TRUE(m.getInteractionTag() == "myTag");
}

// EMTripletPairProduction ----------------------------------------------------
TEST(EMTripletPairProduction, allBackgrounds) {
	// Test if interaction data files are loaded.
	ref_ptr<PhotonField> cmb = new CMB();
	EMTripletPairProduction em(cmb);
	ref_ptr<PhotonField> ebl = new IRB_Kneiske04();
	em.setPhotonField(ebl);
	ref_ptr<PhotonField> urb = new URB_Protheroe96();
	em.setPhotonField(urb);
	ebl = new IRB_Stecker05();
	em.setPhotonField(ebl);
	ebl = new IRB_Franceschini08();
	em.setPhotonField(ebl);
	ebl = new IRB_Finke10();
	em.setPhotonField(ebl);
	ebl = new IRB_Dominguez11();
	em.setPhotonField(ebl);
	ebl = new IRB_Gilmore12();
	em.setPhotonField(ebl);
	ebl = new IRB_Stecker16_upper();
	em.setPhotonField(ebl);
	ebl = new IRB_Stecker16_lower();
	em.setPhotonField(ebl);
	ebl = new IRB_Finke22();
	em.setPhotonField(ebl);
	urb = new URB_Fixsen11();
	em.setPhotonField(urb);
	urb = new URB_Nitu21();
	em.setPhotonField(urb);
}

TEST(EMTripletPairProduction, limitNextStep) {
	// Test if the interaction limits the next propagation step.
	ref_ptr<PhotonField> cmb = new CMB();
	EMTripletPairProduction m(cmb);
	Candidate c(11, 1E17 * eV);
	c.setNextStep(std::numeric_limits<double>::max());
	m.process(&c);
	EXPECT_LT(c.getNextStep(), std::numeric_limits<double>::max());
}

TEST(EMTripletPairProduction, secondaries) {
	// Test if secondaries are correctly produced.
	ref_ptr<PhotonField> cmb = new CMB();
	ref_ptr<PhotonField> irb = new IRB_Saldana21();
	ref_ptr<PhotonField> urb = new URB_Nitu21();
	EMPairProduction m(cmb);
	m.setHaveElectrons(true);
	m.setThinning(0.);

	std::vector<ref_ptr<PhotonField>> fields;
	fields.push_back(cmb);
	fields.push_back(irb);
	fields.push_back(urb);

	// loop over photon backgrounds
	for (int f = 0; f < fields.size(); f++) {
		m.setPhotonField(fields[f]);
		
		// loop over energies Ep = (1e9 - 1e23) eV
		for (int i = 0; i < 140; i++) {

			double Ep = pow(10, 9.05 + 0.1 * i) * eV;
			Candidate c(11, Ep);
			c.setCurrentStep(1e4 * Mpc); // use lower value so that the test can run faster
			m.process(&c);

			// pass if no interaction has occured (no tabulated rates)
			if (c.current.getEnergy() == Ep)
				continue;

			// expect positive energy of primary electron
			EXPECT_GT(c.current.getEnergy(), 0);
			double Etot = c.current.getEnergy();

			// expect electron / positron with energies 0 < E < Ephoton
			for (int j = 0; j < c.secondaries.size(); j++) {
				Candidate s = *c.secondaries[j];
				EXPECT_EQ(abs(s.current.getId()), 11);
				EXPECT_GT(s.current.getEnergy(), 0);
				EXPECT_LT(s.current.getEnergy(), Ep);
				Etot += s.current.getEnergy();
			}

			// test energy conservation
			EXPECT_NEAR(Ep, Etot, 1e-9);
		}
	}
}

TEST(EMTripletPairProduction, interactionTag) {
	EMTripletPairProduction m(new CMB());

	// test default interactionTag
	EXPECT_TRUE(m.getInteractionTag() == "EMTP");

	// test secondary tag
	m.setHaveElectrons(true);
	Candidate c(11, 1 * EeV);
	m.performInteraction(&c);
	EXPECT_TRUE(c.secondaries[0] -> getTagOrigin() == "EMTP");

	// test custom tag
	m.setInteractionTag("myTag");
	EXPECT_TRUE(m.getInteractionTag() == "myTag");
}

// EMInverseComptonScattering -------------------------------------------------
TEST(EMInverseComptonScattering, allBackgrounds) {
	// Test if interaction data files are loaded.
	ref_ptr<PhotonField> cmb = new CMB();
	EMInverseComptonScattering em(cmb);
	ref_ptr<PhotonField> ebl = new IRB_Kneiske04();
	em.setPhotonField(ebl);
	ref_ptr<PhotonField> urb = new URB_Protheroe96();
	em.setPhotonField(urb);
	ebl = new IRB_Stecker05();
	em.setPhotonField(ebl);
	ebl = new IRB_Franceschini08();
	em.setPhotonField(ebl);
	ebl = new IRB_Finke10();
	em.setPhotonField(ebl);
	ebl = new IRB_Dominguez11();
	em.setPhotonField(ebl);
	ebl = new IRB_Gilmore12();
	em.setPhotonField(ebl);
	ebl = new IRB_Stecker16_upper();
	em.setPhotonField(ebl);
	ebl = new IRB_Stecker16_lower();
	em.setPhotonField(ebl);
	ebl = new IRB_Finke22();
	em.setPhotonField(ebl);
	urb = new URB_Fixsen11();
	em.setPhotonField(urb);
	urb = new URB_Nitu21();
	em.setPhotonField(urb);
}

TEST(EMInverseComptonScattering, limitNextStep) {
	// Test if the interaction limits the next propagation step.
	ref_ptr<PhotonField> cmb = new CMB();
	EMInverseComptonScattering m(cmb);
	Candidate c(11, 1E17 * eV);
	c.setNextStep(std::numeric_limits<double>::max());
	m.process(&c);
	EXPECT_LT(c.getNextStep(), std::numeric_limits<double>::max());
}

TEST(EMInverseComptonScattering, secondaries) {
	// Test if secondaries are correctly produced.
	ref_ptr<PhotonField> cmb = new CMB();
	ref_ptr<PhotonField> irb = new IRB_Saldana21();
	ref_ptr<PhotonField> urb = new URB_Nitu21();
	EMPairProduction m(cmb);
	m.setHaveElectrons(true);
	m.setThinning(0.);

	std::vector<ref_ptr<PhotonField>> fields;
	fields.push_back(cmb);
	fields.push_back(irb);
	fields.push_back(urb);

	// loop over photon backgrounds
	for (int f = 0; f < fields.size(); f++) {
		m.setPhotonField(fields[f]);
		
		// loop over energies Ep = (1e9 - 1e23) eV
		for (int i = 0; i < 140; i++) {
			double Ep = pow(10, 9.05 + 0.1 * i) * eV;
			Candidate c(11, Ep);
			c.setCurrentStep(1e3 * Mpc); // use lower value so that the test can run faster
			m.process(&c);

			// pass if no interaction has occured (no tabulated rates)
			if (c.current.getEnergy() == Ep)
				continue;
			
			// expect positive energy of primary electron
			EXPECT_GT(c.current.getEnergy(), 0);

			// expect photon with energy 0 < E < Ephoton
			Candidate s = *c.secondaries[0];
			EXPECT_EQ(abs(s.current.getId()), 22);
			EXPECT_TRUE(s.current.getEnergy() >= 0.);
			EXPECT_TRUE(s.current.getEnergy() < Ep);


			double Etot = c.current.getEnergy();
			for (int j = 0; j < c.secondaries.size(); j++) {
				s = *c.secondaries[j];
				Etot += s.current.getEnergy();
			}
			EXPECT_NEAR(Ep, Etot, 1e-9); 
		}
	}
}

TEST(EMInverseComptonScattering, interactionTag) {
	EMInverseComptonScattering m(new CMB());

	// test default interactionTag
	EXPECT_TRUE(m.getInteractionTag() == "EMIC");

	// test secondary tag
	m.setHavePhotons(true);
	Candidate c(11, 1 * PeV);
	m.performInteraction(&c);
	EXPECT_TRUE(c.secondaries[0] -> getTagOrigin() == "EMIC");

	// test custom tag
	m.setInteractionTag("myTag");
	EXPECT_TRUE(m.getInteractionTag() == "myTag");
}

// SynchrotronRadiation -------------------------------------------------
TEST(SynchrotronRadiation, interactionTag) {
	SynchrotronRadiation s(1 * muG, true);

	// test default interactionTag
	EXPECT_TRUE(s.getInteractionTag() == "SYN");

	// test secondary tag
	Candidate c(11, 10 * PeV);
	c.setCurrentStep(1 * pc);
	s.process(&c);
	EXPECT_TRUE(c.secondaries[0] -> getTagOrigin() == "SYN");

	// test custom tag
	s.setInteractionTag("myTag");
	EXPECT_TRUE(s.getInteractionTag() == "myTag");
}

TEST(SynchrotronRadiation, simpleTestRMS) {
	// test initialisation with B_rms

	// check default values 
	SynchrotronRadiation sync;

	EXPECT_EQ(sync.getBrms(), 0);
	EXPECT_FALSE(sync.getHavePhotons());
	EXPECT_EQ(sync.getThinning(), 0);
	EXPECT_EQ(sync.getLimit(), 0.1);
	EXPECT_EQ(sync.getMaximumSamples(), 0);
	EXPECT_EQ(sync.getSecondaryThreshold(), 1 * MeV);

	// init with custom values 
	double b = 1 * muG; 
	double thinning = 0.23;
	int samples = 4; 
	double limit = 0.123;
	SynchrotronRadiation sync2(b, true, thinning, samples, limit);

	EXPECT_EQ(sync2.getBrms(), b);
	EXPECT_TRUE(sync2.getHavePhotons());
	EXPECT_EQ(sync2.getThinning(), thinning);
	EXPECT_EQ(sync2.getLimit(), limit);
	EXPECT_EQ(sync2.getMaximumSamples(), samples);
	EXPECT_EQ(sync2.getSecondaryThreshold(), 1 * MeV);
}

TEST(SynchrotronRadiation, simpleTestField) {
	// test initialisation with field 

	// check default values 
	Vector3d b(0, 0, 1 * muG);
	ref_ptr<MagneticField> field = new UniformMagneticField(b);
	SynchrotronRadiation sync(field);

	EXPECT_EQ(sync.getBrms(), 0);
	EXPECT_FALSE(sync.getHavePhotons());
	EXPECT_EQ(sync.getThinning(), 0);
	EXPECT_EQ(sync.getLimit(), 0.1);
	EXPECT_EQ(sync.getMaximumSamples(), 0);
	EXPECT_EQ(sync.getSecondaryThreshold(), 1 * MeV);
	Vector3d fieldAtPosition = sync.getField() -> getField(Vector3d(1, 2 , 3));
	EXPECT_EQ(fieldAtPosition.getR(), b.getR());

	// init with custom values 
	double thinning = 0.23;
	int samples = 4; 
	double limit = 0.123;
	SynchrotronRadiation sync2(field, true, thinning, samples, limit);

	EXPECT_EQ(sync2.getBrms(), 0);
	EXPECT_TRUE(sync2.getHavePhotons());
	EXPECT_EQ(sync2.getThinning(), thinning);
	EXPECT_EQ(sync2.getLimit(), limit);
	EXPECT_EQ(sync2.getMaximumSamples(), samples);
	EXPECT_EQ(sync2.getSecondaryThreshold(), 1 * MeV);
	fieldAtPosition = sync2.getField() -> getField(Vector3d(1, 2 , 3));
	EXPECT_EQ(fieldAtPosition.getR(), b.getR());
}

TEST(SynchrotronRadiation, getSetFunctions) {
	SynchrotronRadiation sync;

	// have photons
	sync.setHavePhotons(true);
	EXPECT_TRUE(sync.getHavePhotons());

	// Brms 
	sync.setBrms(5 * muG);
	EXPECT_EQ(sync.getBrms(), 5 * muG);

	// thinning 
	sync.setThinning(0.345);
	EXPECT_EQ(sync.getThinning(), 0.345);

	// limit
	sync.setLimit(0.234);
	EXPECT_EQ(sync.getLimit(), 0.234);

	// max samples
	sync.setMaximumSamples(12345);
	EXPECT_EQ(sync.getMaximumSamples(), 12345);

	// field 
	Vector3d b(1,2,3);
	ref_ptr<MagneticField> field = new UniformMagneticField(b);
	sync.setField(field);
	EXPECT_TRUE(field == sync.getField()); // same pointer

	// secondary threshold
	sync.setSecondaryThreshold(1 * eV); 
	EXPECT_EQ(sync.getSecondaryThreshold(), 1 * eV);
}

TEST(SynchrotronRadiation, energyLoss) {
	double brms = 1 * muG; 
	double step = 1 * kpc; 
	SynchrotronRadiation sync(brms, false);

	double dE, lf, Rg, dEdx;
	Candidate c(11); 
	c.setCurrentStep(step);
	c.setNextStep(step);
	double charge = eplus;

	// 1 GeV 
	c.current.setEnergy(1 * GeV);
	lf = c.current.getLorentzFactor();
	Rg = 1 * GeV / charge / c_light / (brms * sqrt(2. / 3) ); // factor 2/3 for avg magnetic field direction.  
	dEdx = 1. / 6 / M_PI / epsilon0 * pow(lf * lf - 1, 2) * pow(charge / Rg, 2); // Jackson p. 770 (14.31)
	dE = dEdx * step;
	sync.process(&c);
	EXPECT_NEAR(1 * GeV - c.current.getEnergy(), dE, 0.01 * dE);

	// 100 GeV
	c.current.setEnergy(100 * GeV);
	lf = c.current.getLorentzFactor();
	Rg = 100 * GeV / charge / c_light / (brms * sqrt(2. / 3) ); // factor 2/3 for avg magnetic field direction.  
	dEdx = 1. / 6 / M_PI / epsilon0 * pow(lf * lf - 1, 2) * pow(charge / Rg, 2); // Jackson p. 770 (14.31)
	dE = dEdx * step;
	sync.process(&c);
	EXPECT_NEAR(100 * GeV - c.current.getEnergy(), dE, 0.01 * dE);

	// 10 TeV
	c.current.setEnergy(10 * TeV);
	lf = c.current.getLorentzFactor();
	Rg = 10 * TeV / charge / c_light / (brms * sqrt(2. / 3) ); // factor 2/3 for avg magnetic field direction.  
	dEdx = 1. / 6 / M_PI / epsilon0 * pow(lf * lf - 1, 2) * pow(charge / Rg, 2); // Jackson p. 770 (14.31)
	dE = dEdx * step;
	sync.process(&c);
	EXPECT_NEAR(10 * TeV - c.current.getEnergy(), dE, 0.01 * dE);

	// 1 PeV
	c.current.setEnergy(1 * PeV);
	lf = c.current.getLorentzFactor();
	Rg = 1 * PeV / charge / c_light / (brms * sqrt(2. / 3) ); // factor 2/3 for avg magnetic field direction.  
	dEdx = 1. / 6 / M_PI / epsilon0 * pow(lf * lf - 1, 2) * pow(charge / Rg, 2); // Jackson p. 770 (14.31)
	dE = dEdx * step;
	sync.process(&c);
	EXPECT_NEAR(1 * PeV - c.current.getEnergy(), dE, 0.01 * dE);
}

TEST(SynchrotronRadiation, PhotonEnergy) {
	double brms = 1 * muG; 
	SynchrotronRadiation sync(brms, true);
	sync.setSecondaryThreshold(0.); // allow all secondaries for testing

	double E = 1 * TeV;
	Candidate c(11, E);
	c.setCurrentStep(10 * pc); 
	c.setNextStep(10 * pc);
	
	double lf = c.current.getLorentzFactor();
	double Rg = E / eplus / c_light / (brms * sqrt(2. / 3) ); // factor 2/3 for avg magnetic field direction. 
	double Ecrit = 3. / 4 * h_planck / M_PI * c_light * pow(lf, 3) / Rg;

	sync.process(&c);
	EXPECT_TRUE(c.secondaries.size() > 0);	// must have secondaries

	// check avg energy of the secondary photons 
	double Esec = 0; 
	for (size_t i = 0; i < c.secondaries.size(); i++) {
		Esec += c.secondaries[i] -> current.getEnergy();
	}
	Esec /= c.secondaries.size();

	EXPECT_NEAR(Esec, Ecrit, Ecrit);
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

} // namespace crpropa
