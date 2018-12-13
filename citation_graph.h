#pragma once

#include <vector>
#include <map>
#include <set>
#include <memory>

class PublicationAlreadyCreated : public std::exception {
	const char* what() const noexcept {
		return "PublicationAlreadyCreated";
	}
};

class PublicationNotFound : public std::exception {
	const char* what() const noexcept {
		return "PublicationNotFound";
	}
};

class TriedToRemoveRoot : public std::exception {
	const char* what() const noexcept {
		return "TriedToRemoveRoot";
	}
};

template <typename Publication>
class CitationGraph {
private:
	using id_type = typename Publication::id_type;
	
	struct Node {
		Publication pub;
		std::vector<id_type> par;
		std::vector<id_type> chi;
		Node(const id_type &p)
			: pub(p) {}
	};
	
	std::shared_ptr<Node> root;
	std::map<id_type, std::shared_ptr<Node>> nodes;
	
public:
	// Tworzy nowy graf. Tworzy także węzeł publikacji o identyfikatorze stem_id.
	CitationGraph(const id_type &stem_id) {
		auto nd = nodes.emplace(stem_id, new Node(stem_id)).first->second;
		root = nd;
	}

	// Konstruktor przenoszący i przenoszący operator przypisania. Powinny być
	// noexcept.
	CitationGraph(CitationGraph<Publication> &&other) noexcept;
	CitationGraph<Publication>& operator=(CitationGraph<Publication> &&other) noexcept;
	
	// Próba użycia konstruktora kopiującego lub kopiującego operatora przypisania
	// dla obiektów klasy CitationGraph powinna zakończyć się błędem kompilacji.
	CitationGraph(CitationGraph<Publication>&) = delete;
	CitationGraph<Publication>& operator=(CitationGraph<Publication>&) = delete;
	
	// Zwraca identyfikator źródła. Metoda ta powinna być noexcept wtedy i tylko
	// wtedy, gdy metoda Publication::get_id jest noexcept. Zamiast pytajnika należy
	// wpisać stosowne wyrażenie.
	id_type get_root_id() const noexcept(noexcept(root.get()->pub.get_id())) {
		return root.get()->pub.get_id();
	}

	// Zwraca listę identyfikatorów publikacji cytujących publikację o podanym
	// identyfikatorze. Zgłasza wyjątek PublicationNotFound, jeśli dana publikacja
	// nie istnieje.
	std::vector<id_type> get_children(const id_type &id) const {
		if (!exists(id))
			throw PublicationNotFound();
		// auto &chi = nodes.find(id)->second.get()->chi;
		// return std::vector(chi.begin(), chi.end());
		return nodes.find(id)->second.get()->chi;
	}

	// Zwraca listę identyfikatorów publikacji cytowanych przez publikację o podanym
	// identyfikatorze. Zgłasza wyjątek PublicationNotFound, jeśli dana publikacja
	// nie istnieje.
	std::vector<id_type> get_parents(const id_type &id) const {
		if (!exists(id))
			throw PublicationNotFound();
		// auto &par = nodes.find(id)->second.get()->par;
		// return std::vector(par.begin(), par.end());
		return nodes.find(id)->second.get()->par;
	}

	// Sprawdza, czy publikacja o podanym identyfikatorze istnieje.
	bool exists(const id_type &id) const {
		return nodes.find(id) != nodes.end();
	}

	// Zwraca referencję do obiektu reprezentującego publikację o podanym
	// identyfikatorze. Zgłasza wyjątek PublicationNotFound, jeśli żądana publikacja
	// nie istnieje.
	Publication& operator[](const id_type &id) const {
		if (!exists(id))
			throw PublicationNotFound();
		return nodes.find(id)->second.get()->pub;
	}

	// Tworzy węzeł reprezentujący nową publikację o identyfikatorze id cytującą
	// publikacje o podanym identyfikatorze parent_id lub podanych identyfikatorach
	// parent_ids. Zgłasza wyjątek PublicationAlreadyCreated, jeśli publikacja
	// o identyfikatorze id już istnieje. Zgłasza wyjątek PublicationNotFound, jeśli
	// któryś z wyspecyfikowanych poprzedników nie istnieje.
	void create(const id_type &id, const id_type &parent_id) {
		create(id, std::vector<id_type>{parent_id});
	}
	
	void create(const id_type &id, const std::vector<id_type> &parent_ids) { // TODO: uodpornic sie na wyjatki od id_type
		if (exists(id))
			throw PublicationAlreadyCreated();
		for (auto &i : parent_ids)
			if (!exists(i))
				throw PublicationNotFound();
		
		auto it = nodes.emplace(id, new Node(id)).first;
		
		for (int i = 0; i < (int)parent_ids.size(); i++) {
			try {
				add_citation(id, parent_ids[i]);
			}
			catch(...) {
				// TODO: usunac to co dodalismy do tej pory
				nodes.erase(it);
				throw;
			}
		}
	}

	// Dodaje nową krawędź w grafie cytowań. Zgłasza wyjątek PublicationNotFound,
	// jeśli któraś z podanych publikacji nie istnieje.
	void add_citation(const id_type &child_id, const id_type &parent_id) {
		if (!exists(child_id) || !exists(parent_id))
			throw PublicationNotFound();
		auto &chi = nodes.find(child_id)->second.get()->par;
		auto &par = nodes.find(parent_id)->second.get()->chi;
		
		chi.emplace_back(parent_id);
		try {
			par.emplace_back(child_id);
		}
		catch(...) {
			chi.pop_back();
			throw;
		}
	}

	// Usuwa publikację o podanym identyfikatorze. Zgłasza wyjątek
	// PublicationNotFound, jeśli żądana publikacja nie istnieje. Zgłasza wyjątek
	// TriedToRemoveRoot przy próbie usunięcia pierwotnej publikacji.
	void remove(const id_type &id) { // TODO: uodpornic sie na wyjatki od id_type
		if (!exists(id))
			throw PublicationNotFound();
		if (id == get_root_id())
			throw TriedToRemoveRoot();
		// TODO: trzeba zrobic dfsa i wywalic co trzeba z mapy
		// mozna skopiowac cala strukture przed usuwaniem, zeby uodpornic sie na exceptiony
	}
};
