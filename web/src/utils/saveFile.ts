// Save a Blob to disk. When the browser supports the File System Access API
// (Chromium-based: Chrome/Edge/Opera), open a save dialog so the user can pick
// the folder and file name. Otherwise fall back to a normal anchor download,
// which lands in the browser's default download folder.

interface SaveFilePickerWindow extends Window {
  showSaveFilePicker?: (opts: {
    suggestedName?: string;
    types?: { description: string; accept: Record<string, string[]> }[];
  }) => Promise<{
    createWritable: () => Promise<{
      write: (data: Blob) => Promise<void>;
      close: () => Promise<void>;
    }>;
  }>;
}

/**
 * @returns true if the file was saved (or downloaded), false if the user
 *          cancelled the folder picker.
 */
export async function saveBlob(blob: Blob, suggestedName: string, mime: string): Promise<boolean> {
  const picker = (window as SaveFilePickerWindow).showSaveFilePicker;
  if (typeof picker === "function") {
    const dot = suggestedName.lastIndexOf(".");
    const ext = dot >= 0 ? suggestedName.slice(dot) : "";
    try {
      const handle = await picker({
        suggestedName,
        types: ext ? [{ description: suggestedName, accept: { [mime]: [ext] } }] : undefined,
      });
      const writable = await handle.createWritable();
      await writable.write(blob);
      await writable.close();
      return true;
    } catch (e) {
      // User dismissed the picker → don't fall back to a silent download.
      if (e instanceof DOMException && e.name === "AbortError") return false;
      // Any other failure (e.g. permission) → fall through to the anchor download.
    }
  }

  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = suggestedName;
  a.click();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
  return true;
}
