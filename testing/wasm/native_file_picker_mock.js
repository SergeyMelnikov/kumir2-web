// Exercise Qt's File System Access promise/stream path without native OS UI.
// delayMs: time the "user" takes to choose a file. Real pickers take seconds;
// meanwhile the page keeps receiving events.
window.filePickerTest = {
    files: [], saves: [], openCalls: 0, cancelOpen: false, cancelSave: false,
    failWrite: false, delayMs: 0
};
const filePickerDelay = () => new Promise(resolve => setTimeout(resolve, window.filePickerTest.delayMs));
window.showOpenFilePicker = async function() {
    const state = window.filePickerTest;
    await filePickerDelay();
    state.openCalls++;
    if (state.cancelOpen) {
        state.cancelOpen = false;
        throw new DOMException('Test picker cancelled', 'AbortError');
    }
    return state.files.map(file => ({ getFile: async () => file }));
};
window.showSaveFilePicker = async function(options) {
    const state = window.filePickerTest;
    await filePickerDelay();
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
