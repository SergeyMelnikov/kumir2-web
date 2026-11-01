// Exercise Qt's File System Access promise/stream path without native OS UI.
window.filePickerTest = {
    files: [], saves: [], openCalls: 0, cancelOpen: false, cancelSave: false,
    failWrite: false
};
window.showOpenFilePicker = async function() {
    const state = window.filePickerTest;
    state.openCalls++;
    if (state.cancelOpen) {
        state.cancelOpen = false;
        throw new DOMException('Test picker cancelled', 'AbortError');
    }
    return state.files.map(file => ({ getFile: async () => file }));
};
window.showSaveFilePicker = async function(options) {
    const state = window.filePickerTest;
    if (state.cancelSave) {
        state.cancelSave = false;
        throw new DOMException('Test picker cancelled', 'AbortError');
    }
    const chunks = [];
    return {
        createWritable: async () => ({
            write: async data => {
                if (state.failWrite) {
                    state.failWrite = false;
                    throw new DOMException('Test write failed', 'NotAllowedError');
                }
                chunks.push(new Uint8Array(data).slice());
            },
            close: async () => {
                const bytes = [];
                for (const chunk of chunks)
                    bytes.push(...chunk);
                state.saves.push({ name: options.suggestedName, bytes });
            }
        })
    };
};
