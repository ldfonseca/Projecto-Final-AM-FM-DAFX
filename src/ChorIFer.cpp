
#include "plugin.hpp"

struct ChorIfer : Module {
    enum ParamId {
        DELAY_PARAM,
        DETUNE_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        AM_IN_INPUT,
        FM_IN_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        FM_OUT_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    static const int N = 2;                // 2N+1 = 5 vozes
    static const int MAX_DELAY = 4410;     // 100 ms @ 44.1 kHz (aumentado para melhor sincronização)
    const float VOICE_OFFSET = 0.2f;       // Offset de 20% entre vozes 

    struct Voice {
        float delayBufferFM[MAX_DELAY] = {0};
        float delayBufferAM[MAX_DELAY] = {0};  // Buffer separado para AM sincronizada
        int ptr = 0;
        float phase = 0.f;
    };
    Voice voices[5];

    ChorIfer() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(DELAY_PARAM, 0.f, 7.f, 0.f, "Base Delay", " ms", 0.f, 1.f, 0.1f);
        configParam(DETUNE_PARAM, 0.f, 0.1f, 0.01f, "Detune", "%", 0.f, 100.f, 0.01f);
        configInput(AM_IN_INPUT, "Amplitude (AM)");
        configInput(FM_IN_INPUT, "Frequency (Hz)");
        configOutput(FM_OUT_OUTPUT, "Processed Audio");
    }

    // Função auxiliar para leitura interpolada dos buffers
    float readInterpolatedBuffer(const float buffer[], int ptr, float delaySamples) {
        float readPos = ptr - delaySamples;
        while (readPos < 0) readPos += MAX_DELAY; // Garantir posição positiva
        
        int readIndex = (int)readPos;
        float frac = readPos - readIndex;
        
        int idx0 = readIndex % MAX_DELAY;
        int idx1 = (idx0 + 1) % MAX_DELAY;
        
        return buffer[idx0] * (1.f - frac) + buffer[idx1] * frac;
    }

    void process(const ProcessArgs& args) override {
        float am = inputs[AM_IN_INPUT].getVoltage();
        float fm_hz = inputs[FM_IN_INPUT].getVoltage(); 
        float baseDelay = params[DELAY_PARAM].getValue();
        float detune = params[DETUNE_PARAM].getValue();

        float audioOut = 0.f;

        for (int k = -N; k <= N; k++) {
            int i = k + N;
            Voice& voice = voices[i];

            // 1. Armazena AM/FM nos buffers
            voice.delayBufferAM[voice.ptr] = am;
            voice.delayBufferFM[voice.ptr] = fm_hz;

            // 2. Calcula delay específico para esta voz (Lk = baseDelay*(1 + k*offset))
            float voiceDelay = baseDelay * (1.f + k * VOICE_OFFSET);
            float delaySamples = voiceDelay * 0.001f * args.sampleRate;

            // 3. Leitura interpolada sincronizada (mesmo delay para AM e FM)
            float delayedFreq = readInterpolatedBuffer(voice.delayBufferFM, voice.ptr, delaySamples);
            float delayedAM = readInterpolatedBuffer(voice.delayBufferAM, voice.ptr, delaySamples);

            // 4. Aplica detune (1 + k*D) durante integração de fase
            voice.phase += 2.f * M_PI * delayedFreq * (1.f + k * detune) * args.sampleTime;
            voice.phase = fmod(voice.phase, 2.f * M_PI);

            // 5. Ressíntese com AM sincronizada
            audioOut += delayedAM * std::cos(voice.phase);
            
            // Atualiza ponteiro do buffer circular
            voice.ptr = (voice.ptr + 1) % MAX_DELAY;
        }

        // 6. Normalização + ganho fixo (2.5x)
        audioOut = audioOut / (2 * N + 1) * 2.5f; 
        outputs[FM_OUT_OUTPUT].setVoltage(clamp(audioOut, -10.f, 10.f));
    }
};

struct ChorIferWidget : ModuleWidget {
	ChorIferWidget(ChorIfer* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ChorIfer.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(19.900, 35.980)), module, ChorIfer::DELAY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(19.900, 60.400)), module, ChorIfer::DETUNE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.926, 84.300)), module, ChorIfer::FM_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(31.900,84.100)), module, ChorIfer::AM_IN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.000, 108.100)), module, ChorIfer::FM_OUT_OUTPUT));
	}
};


Model* modelChorIfer = createModel<ChorIfer, ChorIferWidget>("ChorIfer");