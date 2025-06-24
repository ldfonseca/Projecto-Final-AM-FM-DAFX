
#include "plugin.hpp"


struct AMFMAnalyzer : Module {
	// Definição de parâmetros, entradas, saídas, e luzes
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		IN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AM_OUT_OUTPUT,
		FM_OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
  // Configuração do filtro de Hilbert (FIR)
	static const int HILBERT_TAPS = 15;        // Número de coeficientes do filtro FIR
	float hilbertCoeffs[HILBERT_TAPS];         // Coeficientes do filtro
	float inputBuffer[HILBERT_TAPS] = {0};     // Buffer circular para as amostras
	int bufferIndex = 0;                       // Índice atual do buffer
	float prevTheta = 0.f;                     // valor anterior do ângulo de fase

	AMFMAnalyzer() {
		// Configuração inicial do módulo
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(IN_INPUT, "Audio");
		configOutput(AM_OUT_OUTPUT, "AM");
		configOutput(FM_OUT_OUTPUT, "FM(Hz)");

		// Inicializa coeficientes do filtro  Hilbert
	    for(int n = 0; n < HILBERT_TAPS; n++){
		int m = n - (HILBERT_TAPS-1)/2;

		// Formula dos coeficientes do filtro Hilbert(impar: 2(πn), par: 0)
		hilbertCoeffs[n] = (m % 2 == 1)? 2.f /(M_PI * m): 0.f;
	    }
	}

	

	void process(const ProcessArgs& args) override {
		// Leitura de entrda de áudio
		float input = inputs[IN_INPUT].getVoltage();

		// Atualização de buffer circular
		inputBuffer[bufferIndex] = input;
		
		// Calculo da parte imaginária (Transformada de Hilbert)
		float imag = 0.f;
		for(int i =0; i < HILBERT_TAPS; i++){
			int idx = (bufferIndex - i + HILBERT_TAPS) % HILBERT_TAPS;
			imag += hilbertCoeffs[i] * inputBuffer[idx];
		}

		// Parte real compensada
		int realIndex = (bufferIndex - (HILBERT_TAPS - 1)/2 + HILBERT_TAPS)% HILBERT_TAPS; 
		float real = inputBuffer[realIndex];

		bufferIndex = (bufferIndex +1) % HILBERT_TAPS;

		// Saida AM (eq. 5)
		outputs[AM_OUT_OUTPUT].setVoltage(std::sqrt(real*real + imag*imag));
		
		// Saída FM (Eq 6)
		float theta = std::atan2(imag, real);
		float deltaTheta = theta -prevTheta;

		// Desenrolamento de fase para evitar saltos de 2π
		while(deltaTheta >M_PI) deltaTheta -=2*M_PI;
		while(deltaTheta< -M_PI) deltaTheta +=2*M_PI;
		
		outputs[FM_OUT_OUTPUT].setVoltage(deltaTheta * args.sampleRate / (2*M_PI));
		prevTheta = theta;
	}
};

struct AMFMAnalyzerWidget : ModuleWidget {
	AMFMAnalyzerWidget(AMFMAnalyzer* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/AMFMAnalyzer.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.050, 60.280)), module, AMFMAnalyzer::IN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.050, 83.900)), module, AMFMAnalyzer::AM_OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.050, 107.900)), module, AMFMAnalyzer::FM_OUT_OUTPUT));
	}
};


Model* modelAMFMAnalyzer = createModel<AMFMAnalyzer, AMFMAnalyzerWidget>("AMFMAnalyzer");