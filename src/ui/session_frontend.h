#pragma once
#include <vector>

#include "ui.h"

struct DrawRecord;
struct Record;

class SessionFrontend final : public ImGuiWindow
{
public:
	SessionFrontend();
	~SessionFrontend() override;

	void draw() override;
private:


	struct RecordData
	{
		std::vector<std::vector<DrawRecord>> lines;
		float min_value = -10000;
		float max_value = -10000;

		std::vector<DrawRecord>& get_line(int index);
	};

	void compute_record_data(RecordData& output, int level, std::vector<Record>::iterator& it,
		const std::vector<Record>& records);
	void draw_record_data(const SessionFrontend::RecordData& data) const;

	std::vector<Record> last_frame;
	RecordData data;
};
