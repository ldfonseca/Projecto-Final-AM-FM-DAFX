
#include "plugin.hpp"

struct HybridIfer : Module {
    enum ParamId {
        MIX_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        AM_IN_INPUT,
        FM1_IN_INPUT,
        FM2_IN_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        OUT_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };
    
    // Acumulador de fase melhorado
    float integratedPhase = 0.f;
    float prevMixedFM = 0.f;  // Armazenar frequência anterior

    HybridIfer() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(MIX_PARAM, 0.f, 1.f, 0.5f, "Mix");
        configInput(AM_IN_INPUT, "AM Input");
        configInput(FM1_IN_INPUT, "FM1 Input");
        configInput(FM2_IN_INPUT, "FM2 Input");
        configOutput(OUT_OUTPUT, "Audio Out");
    }

    void process(const ProcessArgs& args) override {
        // Lê entradas
        float am = inputs[AM_IN_INPUT].getVoltage();
        float fm1 = inputs[FM1_IN_INPUT].getVoltage(); // em Hz
        float fm2 = inputs[FM2_IN_INPUT].getVoltage(); // em Hz
        float p = params[MIX_PARAM].getValue();

        // Mistura as frequências instantâneas
        float mixedFM = p * fm1 + (1.f - p) * fm2;
        
        // Suavização da frequência para reduzir distorção
        mixedFM = 0.999f * prevMixedFM + 0.001f * mixedFM;
        prevMixedFM = mixedFM;
        
        // Calcula o incremento de fase (integral de frequência)
        float phaseIncrement = 2.f * M_PI * mixedFM * args.sampleTime;
        
        // Atualiza a fase de forma contínua
        integratedPhase += phaseIncrement;
        
        // Ressíntese do sinal com sincronismo de fase melhorado
        float output = am * std::cos(integratedPhase);
        
        // Limitação suave para evitar distorção
        output = std::tanh(output);
        
        outputs[OUT_OUTPUT].setVoltage(output);
    }
};

struct HybridIferWidget : ModuleWidget {
	HybridIferWidget(HybridIfer* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/HybridIfer.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.050, 35.300)), module, HybridIfer::MIX_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.150, 60.100)), module, HybridIfer::AM_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.926, 84.100)), module, HybridIfer::FM1_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(32.050, 84.000)), module, HybridIfer::FM2_IN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(19.900, 108.100)), module, HybridIfer::OUT_OUTPUT));
	}
};


Model* modelHybridIfer = createModel<HybridIfer, HybridIferWidget>("HybridIfer");