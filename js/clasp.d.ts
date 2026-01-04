/**
 * TypeScript definitions for clasp.js
 */

declare namespace clasp {
    type ParamChangeHandler = (id: number, value: number) => void;
    type ParamsSyncHandler = (params: Array<{ id: number; v: number }>) => void;
    type NoteOnHandler = (channel: number, key: number, velocity: number) => void;
    type NoteOffHandler = (channel: number, key: number) => void;
    type MidiCCHandler = (channel: number, cc: number, value: number) => void;
    type ReadyHandler = () => void;
    type GenericHandler = (msg: unknown) => void;

    type EventHandler =
        | ParamChangeHandler
        | ParamsSyncHandler
        | NoteOnHandler
        | NoteOffHandler
        | MidiCCHandler
        | ReadyHandler
        | GenericHandler;

    type DragMoveHandler = (x: number, y: number, dx: number, dy: number) => void;
    type DragEndHandler = () => void;

    /**
     * Subscribe to an event from C++
     */
    function on(event: 'paramChange', handler: ParamChangeHandler): void;
    function on(event: 'paramsSync', handler: ParamsSyncHandler): void;
    function on(event: 'noteOn', handler: NoteOnHandler): void;
    function on(event: 'noteOff', handler: NoteOffHandler): void;
    function on(event: 'midiCC', handler: MidiCCHandler): void;
    function on(event: 'ready', handler: ReadyHandler): void;
    function on(event: string, handler: GenericHandler): void;

    /**
     * Remove an event handler
     */
    function off(event: string, handler: EventHandler): void;

    /**
     * Call a C++ function registered via Protocol::onCall()
     * Returns a Promise that resolves with the result
     */
    function call<T = unknown>(name: string, ...args: unknown[]): Promise<T>;

    /**
     * Send a message to C++ (fire-and-forget)
     */
    function send(type: string, payload?: unknown): void;

    /**
     * Start a drag operation (avoids pointer capture banner)
     */
    function startDrag(onMove: DragMoveHandler, onEnd?: DragEndHandler): void;

    /**
     * End the current drag operation
     */
    function endDrag(): void;

    /**
     * Disable the browser context menu
     */
    function disableContextMenu(): void;
}

declare global {
    interface Window {
        clasp: typeof clasp;
        __clasp_recv: (json: string) => void;
    }
}

export = clasp;
export as namespace clasp;
