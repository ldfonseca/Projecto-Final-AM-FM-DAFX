#include "plugin.hpp"

struct OctIfer : Module {
    enum ParamId {
        PITCH_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        FM_IN_INPUT,
        AM_IN_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        FM_OUT_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    float phase = 0.f;  // Fase acumulada para integração

    OctIfer() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(PITCH_PARAM, 0.f, 1.f, 0.f, "Pitch");
        configInput(FM_IN_INPUT, "Frequency (Hz)");
        configInput(AM_IN_INPUT, "Amplitude (AM)");
        configOutput(FM_OUT_OUTPUT, "Processed Audio");
    }

    void process(const ProcessArgs& args) override {
        // 1. Lê entradas
        float fm_hz = inputs[FM_IN_INPUT].getVoltage();
        float am = inputs[AM_IN_INPUT].getVoltage();
        
        // 2. Mapeia o parâmetro (0-1) para o range 0.5-2.0 (oitava abaixo/acima)
        float pitch = params[PITCH_PARAM].getValue();
        float r = 0.5f + pitch * 1.5f;  // 0.5 no mínimo, 2.0 no máximo
        
        // 3. Integração de fase com transposição (Eq. 10 do documento)
        phase += 2.f * M_PI * fm_hz * r * args.sampleTime;
        if (phase > 2.f * M_PI) {
            phase -= 2.f * M_PI;  // Mantém a fase dentro de 0-2π
        }
        
        // 4. Ressíntese AM/FM
        float audioOut = am * std::cos(phase);
        
        // 5. Envia para saída
        outputs[FM_OUT_OUTPUT].setVoltage(audioOut);
    }
};

struct OctIferWidget : ModuleWidget {
	OctIferWidget(OctIfer* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/OctIfer.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.000, 60.100)), module, OctIfer::PITCH_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.900, 84.000)), module, OctIfer::FM_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(32.100, 84.100)), module, OctIfer::AM_IN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.000, 108.100)), module, OctIfer::FM_OUT_OUTPUT));
	}
};


Model* modelOctIfer = createModel<OctIfer, OctIferWidget>("OctIfer");