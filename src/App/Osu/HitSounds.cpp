#include "HitSounds.h"

#include "Beatmap.h"
#include "ConVar.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "SoundEngine.h"

i32 HitSamples::getNormalSet() {
    if(cv::skin_force_hitsound_sample_set.getInt() > 0) return cv::skin_force_hitsound_sample_set.getInt();

    if(this->normalSet != 0) return this->normalSet;

    auto beatmap = osu->getSelectedBeatmap();
    if(!beatmap) return SampleSetType::NORMAL;

    // Fallback to timing point sample set
    i32 tp_sampleset = beatmap->getTimingPoint().sampleSet;
    if(tp_sampleset != 0) return tp_sampleset;

    // ...Fallback to beatmap sample set
    return beatmap->getDefaultSampleSet();
}

i32 HitSamples::getAdditionSet() {
    if(cv::skin_force_hitsound_sample_set.getInt() > 0) return cv::skin_force_hitsound_sample_set.getInt();

    if(this->additionSet != 0) return this->additionSet;

    // Fallback to normal sample set
    return this->getNormalSet();
}

void HitSamples::play(f32 pan, i32 delta, bool is_sliderslide) {
    auto beatmap = osu->getSelectedBeatmap();
    if(!beatmap) return;

    // Don't play hitsounds when seeking
    if(beatmap->bWasSeekFrame) return;

    if(!cv::sound_panning.getBool() || (cv::mod_fposu.getBool() && !cv::mod_fposu_sound_panning.getBool()) ||
       (cv::mod_fps.getBool() && !cv::mod_fps_sound_panning.getBool())) {
        pan = 0.0f;
    } else {
        pan *= cv::sound_panning_multiplier.getFloat();
    }

    f32 pitch = 0.f;
    if(cv::snd_pitch_hitsounds.getBool()) {
        f32 range = beatmap->getHitWindow100();
        pitch = (f32)delta / range * cv::snd_pitch_hitsounds_factor.getFloat();
    }

    // TODO @kiwec: volume. oh god, mcosu never handled this properly either. oh god.
    // * cv::volume_effects
    // * cv::ignore_beatmap_sample_volume
    // * soundSample.hardcodedVolumeMultiplier
    // * whatever the volume of the hitsound is in current timing point
    // and handle updating already playing sounds... when volume is timing point dependent...

    auto get_default_sound = [is_sliderslide](i32 set, i32 hitSound) {
        std::string sound_name = "SKIN_";

        switch(set) {
            default:
            case SampleSetType::NORMAL:
                sound_name.append("NORMAL");
                break;
            case SampleSetType::SOFT:
                sound_name.append("SOFT");
                break;
            case SampleSetType::DRUM:
                sound_name.append("DRUM");
                break;
        }

        if(is_sliderslide) {
            sound_name.append("SLIDER");
        } else {
            sound_name.append("HIT");
        }

        switch(hitSound) {
            default:
            case HitSoundType::NORMAL:
                sound_name.append(is_sliderslide ? "SLIDE" : "NORMAL");
                break;
            case HitSoundType::WHISTLE:
                sound_name.append("WHISTLE");
                break;
            case HitSoundType::FINISH:
                sound_name.append("FINISH");
                break;
            case HitSoundType::CLAP:
                sound_name.append("CLAP");
                break;
        }

        sound_name.append("_SND");

        return resourceManager->getSound(sound_name);
    };

    auto get_map_sound = [get_default_sound](i32 set, i32 hitSound) {
        // TODO @kiwec: map hitsounds are not supported

        return get_default_sound(set, hitSound);
    };

    std::vector<Sound*> sounds;

    if((this->hitSounds & HitSoundType::NORMAL) || (this->hitSounds == 0)) {
        sounds.push_back(get_map_sound(this->getNormalSet(), HitSoundType::NORMAL));
    }

    if(this->hitSounds & HitSoundType::WHISTLE) {
        sounds.push_back(get_map_sound(this->getAdditionSet(), HitSoundType::WHISTLE));
    }

    if(this->hitSounds & HitSoundType::FINISH) {
        sounds.push_back(get_map_sound(this->getAdditionSet(), HitSoundType::FINISH));
    }

    if(this->hitSounds & HitSoundType::CLAP) {
        sounds.push_back(get_map_sound(this->getAdditionSet(), HitSoundType::CLAP));
    }

    for(auto sound : sounds) {
        if(sound == nullptr) continue;

        if(is_sliderslide) {
            if(!sound->isPlaying()) {
                soundEngine->play(sound, pan, pitch);
            }
        } else {
            soundEngine->play(sound, pan, pitch);
        }
    }
}

void HitSamples::stop() {
    // TODO @kiwec: map hitsounds are not supported

    // NOTE: Timing point might have changed since the time we called play().
    //       So for now we're stopping ALL slider sounds, but in the future
    //       we'll need to store the started sounds somewhere.

    // Bruteforce approach. Will be rewritten when adding map hitsounds.
    auto sets = {"NORMAL", "SOFT", "DRUM"};
    auto types = {"SLIDE", "WHISTLE"};
    for(auto& set : sets) {
        for(auto& type : types) {
            std::string sound_name = "SKIN_";
            sound_name.append(set);
            sound_name.append("SLIDER");
            sound_name.append(type);
            sound_name.append("_SND");

            auto sound = resourceManager->getSound(sound_name);
            if(sound) soundEngine->stop(sound);
        }
    }
}
