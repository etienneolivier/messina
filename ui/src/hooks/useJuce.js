import { useState, useEffect } from 'react';

/**
 * Custom hook to interface with the JUCE WebBrowserComponent Backend.
 * Allows subscribing to parameter changes from C++ and dispatching updates from JS.
 */
export function useJuce(paramId, defaultValue = 0.0) {
    const [value, setValue] = useState(defaultValue);

    useEffect(() => {
        // Register polyfill fallback locally if it doesn't exist yet (for dev without JUCE wrapper)
        if (typeof window.setParameter !== 'function') {
            window.setParameter = (id, val) => console.log(`[Dev] setParameter(${id}, ${val})`);
            window.setParameter.isDev = true;
        }

        const handleJuceEvent = (event) => {
            const data = event.detail !== undefined ? event.detail : event;
            if (data && data.paramId === paramId) {
                setValue(data.value);
            }
        };

        window.addEventListener('juceUpdate', handleJuceEvent);
        if (window.__JUCE__ && window.__JUCE__.backend) {
            window.__JUCE__.backend.addEventListener('juceUpdate', handleJuceEvent);
        }

        window.addEventListener('juceUpdate', handleJuceEvent);
        return () => window.removeEventListener('juceUpdate', handleJuceEvent);
    }, [paramId]);

    const setJuceValue = (newValue) => {
        setValue(newValue);
        if (typeof window.setParameter === 'function' && !window.setParameter.isDev) {
            window.setParameter(paramId, newValue);
        } else if (window.__JUCE__ && typeof window.__JUCE__.getNativeFunction === 'function') {
            const nativeSet = window.__JUCE__.getNativeFunction('setParameter');
            if (nativeSet) nativeSet(paramId, newValue);
        } else if (window.getNativeFunction) {
            window.getNativeFunction('setParameter')(paramId, newValue);
        }
    };

    return [value, setJuceValue];
}
