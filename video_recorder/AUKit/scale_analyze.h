#ifndef SCALE_ANALYZE_H
#define SCALE_ANALYZE_H

#include <vector>
#include <stdlib.h>
#include "mel_types.h"

class ScaleAnalyze
{
private:
	std::vector<t_Scale> m_bsHash;
	std::vector<std::vector<int16_t> > m_score; // f [i][j] means position i with j splits
	std::vector<std::vector<t_Scale> > m_notePartSet;
	int  _NoteCount(t_Scale a);
	void _PushBackNote(int note);
	t_Scale  _XUnion(t_Scale a, t_Scale b, int k);
	std::vector<KeyScale> _GenScale(std::vector<std::vector<int16_t> > splitPath, std::vector<MelodyNote> melodyNoteList);
public:
	std::vector<KeyScale> AnaSplitPos(std::vector<MelodyNote> melodyNoteList);
	ScaleAnalyze();
};

#endif
