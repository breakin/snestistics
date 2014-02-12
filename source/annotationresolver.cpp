#include "annotationresolver.h"
#include <map>

void AnnotationResolver::load(std::istream &input) {
	std::string comment;
	std::string useComment;

	int numLocalLabels = 0;

	std::istream &f = input;

	while (!f.eof()) {
		char buf[4096];
		f.getline(buf, 4096);

		if (strlen(buf) == 0) {
			continue;
		}
		if (buf[0] == ';') {
			if (comment.length() != 0) {
				comment = comment + "\n" + buf;
			}
			else {
				comment = buf;
			}
		}
		else if (buf[0] == '#') {
			useComment = &buf[2];
		}
		else {
			char credit[1024], name[1024];
			char mycomment[1024];

			Annotation a;

			if (sscanf(buf, "function %06X %06X %s %s", &a.startOfRange, &a.endOfRange, name, credit) > 0) {
				a.type = ANNOTATION_FUNCTION;
			} else if (sscanf(buf, "comment %06X \"%[^\"]", &a.startOfRange, mycomment, credit) > 0) {
				m_comments[a.startOfRange] = mycomment;
			} else if (sscanf(buf, "data %06X %06X %s %s", &a.startOfRange, &a.endOfRange, name, credit) >0 ) {
				a.type = ANNOTATION_DATA;
			} else if (sscanf(buf, "ptr16 %06X %06X %s %s", &a.startOfRange, &a.endOfRange, name, credit) >0) {
				a.type = ANNOTATION_POINTERTABLE_16;
			} else if (sscanf(buf, "ptr24 %06X %06X %s %s", &a.startOfRange, &a.endOfRange, name, credit) >0 ) {
				a.type = ANNOTATION_POINTERTABLE_24;
			} else {
				continue;
			}

			a.credit = credit;
			a.name = name;
			a.useComment = useComment;
			a.description = comment;
			
			m_annotations.push_back(a);

			bool foundCollision = false;
			for (size_t p = a.startOfRange; p <= a.endOfRange; p++) {
				if (p + 1 > m_annotationPerStuff.size()) {
					m_annotationPerStuff.resize(p + 1, -1);
				}
				if (!foundCollision && m_annotationPerStuff[p] != -1) {
					foundCollision = true;
					//printf("Collision while reading %06X-%06X, found %06X-%06X\n", pc, pc2, m_annotations[m_annotationPerStuff[p]].startOfRange, m_annotations[m_annotationPerStuff[p]].endOfRange);
				}
				m_annotationPerStuff[p] = m_annotations.size() - 1;
			}
			comment.clear();
			useComment.clear();
		}
	}
}


std::string formatAdress(const Pointer p) {
	char hej[64];
	sprintf(hej, "%06X", p);
	return std::string(hej);
}

std::string protectString(const std::string &str) {
	return "\"" + str + "\"";
}

// It is important to support save if we ever wish to update the format, or need sorting based on something
void AnnotationResolver::save(std::ostream &output) {

	std::map<Pointer, std::pair<const std::string*, const Annotation*>> sorted;

	for (auto it = m_comments.begin(); it != m_comments.end(); ++it) {
		sorted[it->first].first = &it->second;
	}
	for (auto it = m_annotations.begin(); it != m_annotations.end(); ++it) {
		sorted[it->startOfRange].second = &(*it);
	}

	for (auto it = sorted.begin(); it != sorted.end(); ++it) {
		
		const Pointer p(it->first);
		const std::string* c = it->second.first;
		const Annotation *a  = it->second.second;

		if (c != nullptr) {
			output << "comment " << formatAdress(p) << " \"" << *c << "\"" << std::endl;
		}

		if (a != nullptr) {

			if (!a->useComment.empty()) {
				output << "# " << a->useComment << std::endl;
			}
			if (!a->description.empty()) {
				output << a->description << std::endl;
			}

			if (a->type == ANNOTATION_FUNCTION) {
				output << "function " << formatAdress(a->startOfRange) << " " << formatAdress(a->endOfRange) << " " << a->name << " " << a->credit << std::endl;
			}
			else if (a->type == ANNOTATION_POINTERTABLE_16) {
				output << "ptr16 " << formatAdress(a->startOfRange) << " " << formatAdress(a->endOfRange) << " " << a->name << " " << a->credit << std::endl;
			}
			else if (a->type == ANNOTATION_POINTERTABLE_24) {
				output << "ptr24 " << formatAdress(a->startOfRange) << " " << formatAdress(a->endOfRange) << " " << a->name << " " << a->credit << std::endl;
			}
			else if (a->type == ANNOTATION_DATA) {
				output << "data " << formatAdress(a->startOfRange) << " " << formatAdress(a->endOfRange) << " " << a->name << " " << a->credit << std::endl;
			}
		}
	}
}
