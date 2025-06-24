#include "plugin.hpp"

struct WahIfer : Module {
    enum ParamId {
        FREQ_PARAM,    // Frequência central do wah (0.1-10 Hz)
        Q_PARAM,       // Fator de qualidade (0.5-5)
        PARAMS_LEN
    };
    enum InputId {
        FM_IN_INPUT,   // Entrada de frequência instantânea (Hz)
        AM_IN_INPUT,   // Entrada de amplitude
        INPUTS_LEN
    };
    enum OutputId {
        AUDIO_OUTPUT,  // Saída de áudio
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    // Variáveis de estado
    float phase = 0.f;
    float last_filtered_fm = 0.f;
    float x1 = 0.f, x2 = 0.f;  // Histórico de entrada do filtro
    float y1 = 0.f, y2 = 0.f;  // Histórico de saída do filtro

 WahIfer() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(FREQ_PARAM, 0.1f, 10.f, 1.f, "Frequência do Filtro", " Hz");
        configParam(Q_PARAM, 0.5f, 5.f, 1.f, "Fator de Qualidade");
        configInput(FM_IN_INPUT, "Frequency (Hz)");
        configInput(AM_IN_INPUT, "Amplitude (AM)");
        configOutput(AUDIO_OUTPUT, "Processed Audio");
    }

    void process(const ProcessArgs& args) override {
        // 1. Leitura das entradas
        float fm = inputs[FM_IN_INPUT].getVoltage();
        float am = inputs[AM_IN_INPUT].getVoltage();
        float freq = params[FREQ_PARAM].getValue();
        float q = params[Q_PARAM].getValue();
        
        // 2. Limitação para evitar instabilidade
        freq = clamp(freq, 0.1f, 10.f);
        q = clamp(q, 0.5f, 5.f);
        
        // 3. Cálculo dos coeficientes do filtro passa-banda
        float w0 = 2.f * M_PI * freq / args.sampleRate;
        float alpha = std::sin(w0) / (2.f * q);
        float cos_w0 = std::cos(w0);
        
        // Fórmula estável de filtro biquad
        float b0 = alpha;
        float b1 = 0.f;
        float b2 = -alpha;
        float a0 = 1.f + alpha;
        float a1 = -2.f * cos_w0;
        float a2 = 1.f - alpha;
        
        // 4. Aplicação do filtro no sinal FM
        float x0 = clamp(fm, 0.f, 20000.f);  // Frequências válidas
        float filtered_fm = (b0*x0 + b1*x1 + b2*x2 - a1*y1 - a2*y2) / a0;
        
        // Suavização adicional para reduzir ruído
        filtered_fm = 0.7f * filtered_fm + 0.3f * last_filtered_fm;
        last_filtered_fm = filtered_fm;
        
        // Atualização do histórico do filtro
        x2 = x1;
        x1 = x0;
        y2 = y1;
        y1 = clamp(filtered_fm, -1000.f, 1000.f);
        
        // 5. Integração de fase com prevenção de aliasing
        float phase_inc = 2.f * M_PI * filtered_fm * args.sampleTime;
        phase_inc = clamp(phase_inc, -0.5f * M_PI, 0.5f * M_PI);
        
        phase += phase_inc;
        
        // Mantém a fase dentro de [0, 2π]
        if (phase >= 2.f * M_PI) phase -= 2.f * M_PI;
        if (phase < 0.f) phase += 2.f * M_PI;
        
        // 6. Ressíntese AM/FM
        float audio_out = am * std::cos(phase);
        
        // 7. Saída com limitação final
        outputs[AUDIO_OUTPUT].setVoltage(clamp(audio_out * 0.7f, -10.f, 10.f));
    }
};

struct WahIferWidget : ModuleWidget {
	WahIferWidget(WahIfer* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/WahIfer.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.000, 35.400)), module, WahIfer::FREQ_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.150, 60.100)), module, WahIfer::Q_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.926, 84.100)), module, WahIfer::FM_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(32.250, 83.700)), module, WahIfer::AM_IN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.150, 108.100)), module, WahIfer::AUDIO_OUTPUT));
	}
};


Model* modelWahIfer = createModel<WahIfer, WahIferWidget>("WahIfer");